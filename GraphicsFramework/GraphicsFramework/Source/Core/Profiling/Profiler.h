#pragma once

// Profiler.h — Main profiler singleton and public macros.
// This is the primary entry point for all profiling functionality.
// The Profiler class orchestrates CPUProfiler and GPUProfiler subsystems,
// consolidating their data into a unified FrameSnapshot for consumers
// (overlay, CSV export, etc.).

#include "Core/Profiling/ProfilerInternal.h"
#include "Core/Profiling/CPUProfiler.h"
#include "Core/Profiling/GPUProfiler.h"
#include "Core/Profiling/ProfilerOverlay.h"

class Profiler
{
public:
    static Profiler* Instance();

    // Lifecycle
    void Init(const ProfilerSettings& settings = {});
    void Close();

    // Frame management (called by Engine)
    void StartFrame();
    void EndFrame();

    // Timing marks (called by Engine — replaces GF_PROFILING guards in Engine)
    void MarkRenderStart();
    void MarkRenderEnd();
    void MarkImguiStart();
    void MarkImguiEnd();
    void SetWaitTime(double ms);

    // Draw call hooks (called by Renderer)
    void BeginDrawCall(const std::string& objectName, const std::string& shaderName, const std::string& materialName);
    void EndDrawCall();

    // Sets the object name used for draw calls that do not pass one explicitly.
    // ObjectManager calls this around each object's RenderObject().
    void SetDrawCallObjectName(const std::string& name) { mCurrentObjectName = name; }

    // Overlay rendering (called by ImguiManager)
    void RenderOverlay();

    // CSV export
    bool ExportCSV(const std::string& filepath);

    // Accessors
    const ProfilerSettings& GetSettings() const { return mSettings; }
    bool IsEnabled() const { return mEnabled; }

    // Hotkey toggle
    void ToggleEnabled() { mEnabled = !mEnabled; }

    // Overlay visibility
    void SetOverlayVisible(bool visible) { mOverlay.SetVisible(visible); }
    bool IsOverlayVisible() const { return mOverlay.IsVisible(); }

    // Frame counter for CSV naming
    uint32_t GetFrameCount() const { return mFrameCount; }

    // Get the unified frame snapshot (composes CPU + GPU data)
    const FrameSnapshot& GetSnapshot() const { return mFrameSnapshot; }

    // Direct access to individual subsystems (for advanced consumers)
    const CPUProfiler& GetCPUProfiler() const { return *mCPUProfiler; }
    const GPUProfiler& GetGPUProfiler() const { return *mGPUProfiler; }

private:
    Profiler() = default;
    ~Profiler() = default;

    Profiler(const Profiler&) = delete;
    Profiler& operator=(const Profiler&) = delete;

private:
    ProfilerSettings mSettings;
    bool             mEnabled = true;
    bool             mInitialized = false;
    uint32_t         mFrameCount = 0;

    // Unified frame snapshot — composed in EndFrame() from subsystem data.
    FrameSnapshot    mFrameSnapshot;

    // Back-refs to subsystem singletons (set in Init)
    CPUProfiler*     mCPUProfiler = nullptr;
    GPUProfiler*     mGPUProfiler = nullptr;

    // ImGui overlay
    ProfilerOverlay  mOverlay;

    // Object name applied to draw calls that pass an empty object name.
    std::string      mCurrentObjectName;
};

// ─────────────────────────────────────────────────────────────────────────────
// Public macros — these are the user-facing API
// ─────────────────────────────────────────────────────────────────────────────

// Profile a scope (RAII — timing ends when scope exits)
#define GF_PROFILE_SCOPE(name) \
    ScopeGuard gfProfileGuard_##__LINE__(name, __FILE__, __LINE__)

// Profile a one-shot operation
#define GF_PROFILE_ONE_SHOT(name) \
    OneShotGuard gfProfileOneShot_##__LINE__(name)

// Manual begin/end (useful when you can't use RAII)
#define GF_PROFILE_BEGIN(name) \
    ScopeGuard gfProfileBegin_##__LINE__(name, __FILE__, __LINE__)

#define GF_PROFILE_END() \
    /* ScopeGuard destructor handles end timing */

// Profile entire function
#define GF_PROFILE_FUNCTION() \
    GF_PROFILE_SCOPE(__FUNCTION__)
