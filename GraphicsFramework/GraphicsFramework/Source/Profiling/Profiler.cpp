// Profiler.cpp — Main profiler singleton implementation.
// Orchestrates CPUProfiler and GPUProfiler subsystems, consolidating their
// data into a unified FrameSnapshot at EndFrame() for consumers.

#include "Profiling/Profiler.h"

#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <chrono>

// ─────────────────────────────────────────────────────────────────────────────
// Singleton
// ─────────────────────────────────────────────────────────────────────────────
Profiler* Profiler::Instance()
{
    static Profiler instance;
    return &instance;
}

// ─────────────────────────────────────────────────────────────────────────────
// Lifecycle
// ─────────────────────────────────────────────────────────────────────────────
void Profiler::Init(const ProfilerSettings& settings)
{
    if (mInitialized) return;

    mSettings = settings;
    mEnabled = mSettings.enabled;

    mCPUProfiler = CPUProfiler::Instance();
    mCPUProfiler->SetSettings(mSettings);

    mGPUProfiler = GPUProfiler::Instance();
    mGPUProfiler->Init(mSettings.maxConcurrentQueries);

    mInitialized = true;
    mFrameCount = 0;

    std::cout << "[Profiler] Initialized (CPU + GPU, queries=" << mSettings.maxConcurrentQueries << ")" << std::endl;
}

void Profiler::Close()
{
    if (!mInitialized) return;

    mGPUProfiler->Close();
    mInitialized = false;
    mFrameCount = 0;
}

// ─────────────────────────────────────────────────────────────────────────────
// Frame lifecycle
// ─────────────────────────────────────────────────────────────────────────────
void Profiler::StartFrame()
{
    if (!mEnabled || !mInitialized) return;

    mCPUProfiler->StartFrame();
    mGPUProfiler->StartFrame();
}

void Profiler::EndFrame()
{
    if (!mEnabled || !mInitialized) return;

    // End subsystems (each handles its own data collection)
    mCPUProfiler->EndFrame();
    mGPUProfiler->EndFrame();

    // ── Consolidate: compose unified snapshot from both subsystems ──
    mFrameSnapshot = mCPUProfiler->GetSnapshot();  // Copy CPU-side fields + history

    // Override GPU time from GPUProfiler
    mFrameSnapshot.gpuTimeMs = mGPUProfiler->GetFrameGPUTimeMs();

    // Copy draw call samples from GPUProfiler
    if (mSettings.captureDrawCalls)
    {
        mFrameSnapshot.drawCalls = mGPUProfiler->GetDrawCallSamples();
    }

    // Copy CPU samples (already in CPUProfiler's snapshot)
    mFrameSnapshot.cpuSamples = mCPUProfiler->GetSamples();

    mFrameCount++;
}

// ─────────────────────────────────────────────────────────────────────────────
// Timing marks (called by Engine — replaces GF_PROFILING guards in Engine)
// ─────────────────────────────────────────────────────────────────────────────
void Profiler::MarkRenderStart()
{
    if (!mInitialized) return;
    mCPUProfiler->SetRenderStart(std::chrono::high_resolution_clock::now());
}

void Profiler::MarkRenderEnd()
{
    if (!mInitialized) return;
    mCPUProfiler->SetRenderEnd(std::chrono::high_resolution_clock::now());
}

void Profiler::MarkImguiStart()
{
    if (!mInitialized) return;
    mCPUProfiler->SetImguiStart(std::chrono::high_resolution_clock::now());
}

void Profiler::MarkImguiEnd()
{
    if (!mInitialized) return;
    mCPUProfiler->SetImguiEnd(std::chrono::high_resolution_clock::now());
}

void Profiler::SetWaitTime(double ms)
{
    if (!mInitialized) return;
    mCPUProfiler->SetWaitTime(ms);
}

// ─────────────────────────────────────────────────────────────────────────────
// Draw call hooks (called by Renderer)
// ─────────────────────────────────────────────────────────────────────────────
void Profiler::BeginDrawCall(const std::string& objectName, const std::string& shaderName, const std::string& materialName)
{
    if (!mEnabled || !mInitialized || !mSettings.captureDrawCalls) return;

    mGPUProfiler->BeginDrawCall(objectName, shaderName, materialName);
}

void Profiler::EndDrawCall()
{
    if (!mEnabled || !mInitialized) return;

    mGPUProfiler->EndDrawCall();
}

// ─────────────────────────────────────────────────────────────────────────────
// CSV export
// ─────────────────────────────────────────────────────────────────────────────
bool Profiler::ExportCSV(const std::string& filepath)
{
    if (!mInitialized) return false;

    const auto& snapshot = mFrameSnapshot;

    std::ofstream file(filepath);
    if (!file.is_open())
    {
        std::cerr << "[Profiler] Failed to open CSV file: " << filepath << std::endl;
        return false;
    }

    file << std::fixed << std::setprecision(3);

    // ── Frame summary ──
    file << "# Frame " << mFrameCount << std::endl;
    file << "# Frame Time,ms;CPU Time,ms;GPU Time,ms;Render Loop,ms;Update Loop,ms;Imgui,ms;Wait,ms" << std::endl;
    file << snapshot.frameTimeMs << ";"
         << snapshot.cpuTimeMs << ";"
         << snapshot.gpuTimeMs << ";"
         << snapshot.renderLoopTimeMs << ";"
         << snapshot.updateLoopTimeMs << ";"
         << snapshot.imguiTimeMs << ";"
         << snapshot.waitTimeMs << std::endl;
    file << std::endl;

    // ── CPU Samples (grouped) ──
    file << "# CPU Samples (grouped by name)" << std::endl;
    file << "Name;Count;Total Time,ms;Avg Time,ms" << std::endl;
    for (const auto& sample : snapshot.cpuSamples)
    {
        file << sample.name << ";"
             << sample.count << ";"
             << sample.totalTime << ";"
             << sample.avgTime << std::endl;
    }
    file << std::endl;

    // ── Draw Calls ──
    file << "# Draw Calls" << std::endl;
    file << "Instance;Object;Shader;Material;GPU Time,ms" << std::endl;
    for (const auto& draw : snapshot.drawCalls)
    {
        file << draw.instanceID << ";"
             << draw.objectName << ";"
             << draw.shaderName << ";"
             << draw.materialName << ";"
             << draw.gpuTimeMs << std::endl;
    }
    file << std::endl;

    // ── Frame History ──
    file << "# Frame History (last " << snapshot.frameTimeHistory.size() << " frames)" << std::endl;
    file << "FrameIndex;FrameTime,ms" << std::endl;
    for (size_t i = 0; i < snapshot.frameTimeHistory.size(); ++i)
    {
        file << (mFrameCount - snapshot.frameTimeHistory.size() + i) << ";"
             << snapshot.frameTimeHistory[i] << std::endl;
    }

    file.close();
    std::cout << "[Profiler] Exported CSV: " << filepath << " ("
              << snapshot.cpuSamples.size() << " CPU samples, "
              << snapshot.drawCalls.size() << " draw calls)" << std::endl;

    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// Overlay rendering
// ─────────────────────────────────────────────────────────────────────────────
void Profiler::RenderOverlay()
{
    if (!mEnabled || !mInitialized || !mOverlay.IsVisible()) return;

    mOverlay.Render(*this);
}
