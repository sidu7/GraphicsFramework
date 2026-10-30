// CPUProfiler.cpp — RAII CPU profiler with sample grouping and frame tracking.

#include "Core/Profiling/CPUProfiler.h"

#include <algorithm>
#include <iostream>
#include <unordered_map>

// ─────────────────────────────────────────────────────────────────────────────
// Singleton interface
// ─────────────────────────────────────────────────────────────────────────────
void CPUProfiler::StartFrame()
{
    mFrameStart = std::chrono::high_resolution_clock::now();
    mRawSamples.clear();
    mScopes.clear();
    mSamples.clear();

    // Keep frameTimeHistory across frames; only clear the per-frame fields.
    mSnapshot.cpuSamples.clear();
    mSnapshot.drawCalls.clear();
    mSnapshot.frameTimeMs = 0.0;
    mSnapshot.cpuTimeMs = 0.0;
    mSnapshot.gpuTimeMs = 0.0;
    mSnapshot.renderLoopTimeMs = 0.0;
    mSnapshot.updateLoopTimeMs = 0.0;
    mSnapshot.imguiTimeMs = 0.0;
    mSnapshot.waitTimeMs = 0.0;
}

void CPUProfiler::EndFrame()
{
    auto frameEnd = std::chrono::high_resolution_clock::now();

    // Total frame time (wall clock)
    double frameTime = std::chrono::duration<double, std::milli>(frameEnd - mFrameStart).count();
    mSnapshot.frameTimeMs = frameTime;

    // Compute render and imgui durations from time points
    if (mRenderStart != std::chrono::high_resolution_clock::time_point{})
    {
        mSnapshot.renderLoopTimeMs = std::chrono::duration<double, std::milli>(mRenderEnd - mRenderStart).count();
    }
    if (mImguiStart != std::chrono::high_resolution_clock::time_point{})
    {
        mSnapshot.imguiTimeMs = std::chrono::duration<double, std::milli>(mImguiEnd - mImguiStart).count();
    }

    // Group raw samples into named totals (must happen before cpuTimeMs uses mSamples)
    GroupSamples();

    // CPU loading = time spent in the main loop (render + update + imgui + wait)
    // Use max(explicit sum, largest sample) to avoid double-counting:
    // GF_PROFILE_SCOPE samples include the explicit sections, so the larger value
    // represents the true dominant CPU bottleneck without inflating the sum.
    const double explicitSum = mSnapshot.renderLoopTimeMs + mSnapshot.updateLoopTimeMs
                             + mSnapshot.imguiTimeMs + mSnapshot.waitTimeMs;

    // Find the largest grouped sample total time
    double largestSampleMs = 0.0;
    for (const auto& s : mSamples)
    {
        if (s.totalTime > largestSampleMs)
        {
            largestSampleMs = s.totalTime;
        }
    }
    mSnapshot.cpuTimeMs = (explicitSum > largestSampleMs) ? explicitSum : largestSampleMs;

    // NOTE: GPU data (gpuTimeMs, drawCalls) is NOT populated here.
    // It is filled in by Profiler::EndFrame() which consolidates both subsystems.

    // Update rolling history (CPU-side only; Profiler may replace frameTimeMs with consolidated value)
    mSnapshot.frameTimeHistory.push_back(frameTime);
    if (static_cast<int>(mSnapshot.frameTimeHistory.size()) > FrameSnapshot::MaxHistory)
    {
        mSnapshot.frameTimeHistory.erase(
            mSnapshot.frameTimeHistory.begin(),
            mSnapshot.frameTimeHistory.begin() + (mSnapshot.frameTimeHistory.size() - FrameSnapshot::MaxHistory)
        );
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Scope management (called by ScopeGuard)
// ─────────────────────────────────────────────────────────────────────────────
void CPUProfiler::PushScope(const char* name)
{
    ScopeTimer timer;
    timer.name = name;
    timer.start = std::chrono::high_resolution_clock::now();
    mScopes.push_back(std::move(timer));
}

void CPUProfiler::PopScope()
{
    if (mScopes.empty()) return;

    auto& scope = mScopes.back();
    auto end = std::chrono::high_resolution_clock::now();
    double elapsed = std::chrono::duration<double, std::milli>(end - scope.start).count();

    // Optionally filter by duration
    if (mSettings.maxCPUSampleDurationMs > 0.0 && elapsed < mSettings.maxCPUSampleDurationMs)
    {
        mScopes.pop_back();
        return;
    }

    // Build full scope path from parent scopes (below current on stack).
    std::string fullPath;
    for (size_t i = 0; i < mScopes.size() - 1; ++i)
    {
        if (i > 0) fullPath += " > ";
        fullPath += mScopes[i].name;
    }

    // Append the current scope (being recorded) to form the complete path.
    if (!fullPath.empty()) fullPath += " > ";
    fullPath += scope.name;

    // Store raw sample
    CPUSample sample;
    sample.name = scope.name;
    sample.path  = std::move(fullPath);
    sample.count = 1;
    sample.totalTime = elapsed;
    sample.avgTime = elapsed;

    mRawSamples.push_back(sample);
    mScopes.pop_back();
}

// ─────────────────────────────────────────────────────────────────────────────
// One-shot recording
// ─────────────────────────────────────────────────────────────────────────────
void CPUProfiler::RecordOneShot(const std::string& name, double elapsedMs)
{
    if (mSettings.maxCPUSampleDurationMs > 0.0 && elapsedMs < mSettings.maxCPUSampleDurationMs)
        return;

    CPUSample sample;
    sample.name = name;
    sample.count = 1;
    sample.totalTime = elapsedMs;
    sample.avgTime = elapsedMs;
    mRawSamples.push_back(sample);
}

// ─────────────────────────────────────────────────────────────────────────────
// Group samples by path and build parent-child hierarchy
// ─────────────────────────────────────────────────────────────────────────────
void CPUProfiler::GroupSamples()
{
    // 1. Group by full path (same as name when no nested scopes).
    std::unordered_map<std::string, CPUSample> groupMap;
    for (const auto& raw : mRawSamples)
    {
        auto it = groupMap.find(raw.path);
        if (it != groupMap.end())
        {
            it->second.count       += raw.count;
            it->second.totalTime   += raw.totalTime;
        }
        else
        {
            groupMap[raw.path] = raw;
        }
    }

    // 2. Build indexed vector so we can reference samples by index.
    mSamples.clear();
    mSamples.reserve(groupMap.size());
    for (auto& pair : groupMap)
    {
        auto& sample = pair.second;
        sample.avgTime  = (sample.count > 0) ? (sample.totalTime / sample.count) : 0.0;
        sample.parentIndex = -1;  // default: root-level
        mSamples.push_back(sample);
    }

    // 3. Sort by total time (descending) — most expensive first.
    //    Must happen BEFORE building parent indices so the index map stays valid.
    std::sort(mSamples.begin(), mSamples.end(),
        [](const CPUSample& a, const CPUSample& b)
        {
            return a.totalTime > b.totalTime;
        });

    // 4. For each sample, find its parent by stripping the last path component.
    //    Example: "Engine::Update > RenderLoop" -> parent path = "Engine::Update"
    std::unordered_map<std::string, int> pathToIndex;
    for (int i = 0; i < static_cast<int>(mSamples.size()); ++i)
    {
        pathToIndex[mSamples[i].path] = i;
    }

    for (auto& sample : mSamples)
    {
        auto sepPos = sample.path.rfind(" > ");
        if (sepPos != std::string::npos)
        {
            std::string parentPath = sample.path.substr(0, sepPos);
            auto it = pathToIndex.find(parentPath);
            if (it != pathToIndex.end())
            {
                sample.parentIndex = it->second;
            }
        }
    }
}
