#pragma once

// CPUProfiler.h — RAII-based CPU profiling with scope guards and sample grouping.

#include "Core/Profiling/ProfilerInternal.h"
#include "Core/Singleton.h"
#include <string>
#include <vector>
#include <chrono>

class CPUProfiler : public Singleton<CPUProfiler>
{
public:
    void StartFrame();
    void EndFrame();

    // Record a one-shot timing
    void RecordOneShot(const std::string& name, double elapsedMs);

    // Accessors (called from Profiler/Overlay)
    const std::vector<CPUSample>& GetSamples() const { return mSamples; }
    double GetCurrentCPULoading() const { return mSnapshot.cpuTimeMs; }
    double GetRenderLoopTime() const { return mSnapshot.renderLoopTimeMs; }
    double GetUpdateLoopTime() const { return mSnapshot.updateLoopTimeMs; }
    double GetImguiTime() const      { return mSnapshot.imguiTimeMs; }
    double GetWaitTime() const       { return mSnapshot.waitTimeMs; }

    // Configuration
    void SetSettings(const ProfilerSettings& settings) { mSettings = settings; }

    // Timing mark setters (called by Profiler)
    void SetRenderStart(std::chrono::high_resolution_clock::time_point start) { mRenderStart = start; }
    void SetRenderEnd(std::chrono::high_resolution_clock::time_point end)     { mRenderEnd = end; }
    void SetImguiStart(std::chrono::high_resolution_clock::time_point start) { mImguiStart = start; }
    void SetImguiEnd(std::chrono::high_resolution_clock::time_point end)     { mImguiEnd = end; }

    // Set durations from external sources (e.g. Engine)
    void SetUpdateLoopTime(double ms)  { mSnapshot.updateLoopTimeMs = ms; }
    void SetWaitTime(double ms)        { mSnapshot.waitTimeMs = ms; }

    // Get the CPU-side frame snapshot (CPU-only data; GPU fields will be zero/empty)
    const FrameSnapshot& GetSnapshot() const { return mSnapshot; }

    // Scope management (called by ScopeGuard/OneShotGuard)
    void PushScope(const char* name);
    void PopScope();

private:
    friend class Singleton<CPUProfiler>;
    friend class ScopeGuard;
    friend class OneShotGuard;
    CPUProfiler() = default;
    ~CPUProfiler() = default;

    void GroupSamples();

private:
    // Per-invocation timer stack (scoped)
    struct ScopeTimer
    {
        std::string name;
        std::chrono::high_resolution_clock::time_point start;
    };

    // Frame data — CPUProfiler only manages CPU-side fields.
    // GPU-side fields (gpuTimeMs, drawCalls) are populated by Profiler::EndFrame().
    std::vector<ScopeTimer>    mScopes;              // Active scopes (stack per thread)
    std::vector<CPUSample>     mRawSamples;          // Individual invocations before grouping
    std::vector<CPUSample>     mSamples;             // Grouped output
    FrameSnapshot              mSnapshot;
    ProfilerSettings           mSettings;

    // Frame start timestamp
    std::chrono::high_resolution_clock::time_point mFrameStart;

    // Timing mark points (set by Profiler layer)
    std::chrono::high_resolution_clock::time_point mRenderStart;
    std::chrono::high_resolution_clock::time_point mRenderEnd;
    std::chrono::high_resolution_clock::time_point mImguiStart;
    std::chrono::high_resolution_clock::time_point mImguiEnd;
};

// ─────────────────────────────────────────────────────────────────────────────
// ScopeGuard / OneShotGuard — RAII classes for CPU profiling.
// Public macros (GF_PROFILE_SCOPE, etc.) live in Profiler.h so applications
// only need to #include <Core/Profiling/Profiler.h>.
// ─────────────────────────────────────────────────────────────────────────────

class ScopeGuard
{
public:
    ScopeGuard(const char* name, const char* file, int line)
        : mName(name ? name : "unnamed")
        , mFile(file)
        , mLine(line)
    {
        // Only use the name — file:line prefixes are redundant and truncate
        // the actual function/scope name in the overlay.
        CPUProfiler::Instance()->PushScope(mName.c_str());
    }

    ~ScopeGuard()
    {
        CPUProfiler::Instance()->PopScope();
    }

    ScopeGuard(const ScopeGuard&) = delete;
    ScopeGuard& operator=(const ScopeGuard&) = delete;
    ScopeGuard(ScopeGuard&&) = delete;
    ScopeGuard& operator=(ScopeGuard&&) = delete;

private:
    std::string mName;
    const char* mFile;
    int         mLine;
};

class OneShotGuard
{
public:
    explicit OneShotGuard(const std::string& name)
        : mName(name)
        , mStart(std::chrono::high_resolution_clock::now())
    {}

    ~OneShotGuard()
    {
        auto end = std::chrono::high_resolution_clock::now();
        double elapsed = std::chrono::duration<double, std::milli>(end - mStart).count();
        CPUProfiler::Instance()->RecordOneShot(mName, elapsed);
    }

private:
    std::string mName;
    std::chrono::high_resolution_clock::time_point mStart;
};
