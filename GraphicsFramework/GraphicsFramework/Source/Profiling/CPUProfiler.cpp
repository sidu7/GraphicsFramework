// CPUProfiler.cpp — RAII CPU profiler with sample grouping and frame tracking.

#include "Profiling/CPUProfiler.h"

#include <algorithm>
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
    mSnapshot = FrameSnapshot{};
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

    // CPU loading = time spent in the main loop (render + update + imgui + wait)
    mSnapshot.cpuTimeMs = mSnapshot.renderLoopTimeMs + mSnapshot.updateLoopTimeMs
                        + mSnapshot.imguiTimeMs + mSnapshot.waitTimeMs;

    // NOTE: GPU data (gpuTimeMs, drawCalls) is NOT populated here.
    // It is filled in by Profiler::EndFrame() which consolidates both subsystems.

    // Group raw samples
    GroupSamples();

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

    // Store raw sample
    CPUSample sample;
    sample.name = scope.name;
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
// Group samples by name
// ─────────────────────────────────────────────────────────────────────────────
void CPUProfiler::GroupSamples()
{
    // Use unordered_map for grouping
    std::unordered_map<std::string, CPUSample> groupMap;

    for (const auto& raw : mRawSamples)
    {
        auto it = groupMap.find(raw.name);
        if (it != groupMap.end())
        {
            it->second.count += raw.count;
            it->second.totalTime += raw.totalTime;
        }
        else
        {
            groupMap[raw.name] = raw;
        }
    }

    // Calculate averages and build final vector
    mSamples.clear();
    mSamples.reserve(groupMap.size());
    for (auto& pair : groupMap)
    {
        auto& sample = pair.second;
        sample.avgTime = (sample.count > 0) ? (sample.totalTime / sample.count) : 0.0;
        mSamples.push_back(sample);
    }

    // Sort by total time (descending) — most expensive first
    std::sort(mSamples.begin(), mSamples.end(),
        [](const CPUSample& a, const CPUSample& b)
        {
            return a.totalTime > b.totalTime;
        });
}
