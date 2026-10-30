# GraphicsFramework Profiler — Implementation Plan

**Goal:** Add a CPU + GPU profiler to GraphicsFramework that both framework internals and user scenes can use. It provides RAII scope guards for CPU profiling, per-draw-call GPU timing (Vulkan + OpenGL), an ImGui overlay for real-time inspection, and CSV export.

**Constraints:**
- Windows x64 + MSVC only
- **Always compiled** — no CMake option or conditional compilation; profiler is unconditionally linked into every build configuration.

---

## Implementation Status (as of 2026-10-29)

| Phase | Goal | Status | Details |
|---|---|---|---|
| **0 — Foundation** | Skeleton + CMake | ✅ **Complete** | All 8 skeleton files created; `GF_BUILD_DEBUG_PROFILER` CMake option removed; profiler always compiled |
| **1 — CPU Profiler** | RAII guard + macro API + frame hooks + Engine integration | ✅ **Complete** | `ScopeGuard` / `OneShotGuard` RAII classes, 4 macros (`GF_PROFILE_SCOPE`, `GF_PROFILE_ONE_SHOT`, `GF_PROFILE_BEGIN`, `GF_PROFILE_FUNCTION`), `Engine::Run()` hooks (`StartFrame`, `MarkRenderStart/End`, `MarkImguiStart/End`, `EndFrame`), `Settings::Parse()` for profiler JSON, `Profiler::Close()` in `Engine::Stop()` |
| **2 — GPU Profiler** | Per-draw-call GPU timing on both backends | ✅ **Complete** | Query pool ring buffer, OpenGL `GL_ARB_timer_query` backend, Vulkan `VK_EXT_calibrated_timestamps` support, draw call hooks in `DeviceGL.cpp` and `DeviceVK.cpp` |
| **3 — ImGui Overlay** | Real-time visualization + CSV export | ✅ **Complete** | `RenderFrameGraph()` (rolling bar chart), `RenderCPUSamples()` (table), `RenderDrawCalls()` (scrollable list), `RenderExportButton()` (CSV with `std::ofstream`), F11 keyboard toggle in `ImguiManager::Update()` |
| **4 — Integration & Polish** | Framework profiling, overflow handling, Settings.json defaults | ✅ **Complete** | `GF_PROFILE_SCOPE` added to shader init, uniform buffer, and texture init methods (OpenGL + Vulkan); query pool overflow warning added; frame history ring buffer implemented; Settings.json updated in all 5 samples |

### Resolved Issues

| # | Issue | Severity | Phase | Resolution |
|---|---|---|---|---|
| 1 | CMake `GF_PROFILING` defined for Release when `GF_BUILD_DEBUG_PROFILER` is ON | **High** | 0 | ✅ **Resolved** — `GF_BUILD_DEBUG_PROFILER` option removed entirely; profiler always compiled |
| 2 | `Profiler::Instance()->Close()` never called | **Medium** | 1 | ✅ **Resolved** — added `Profiler::Instance()->Close()` unconditionally in `Engine::Stop()` |
| 3 | Vulkan `InitVulkan()` is an empty stub | **High** | 2 | ✅ **Resolved** — implemented query pool creation, `VK_EXT_calibrated_timestamps` detection and loading, calibrated GPU clock timestamps |
| 4 | No F11 keyboard hotkey | **Low** | 3 | ✅ **Resolved** — added `ImGui::IsKeyPressed(ImGuiKey_F11)` check in `ImguiManager::Update()` |
| 5 | No draw call hooks in `Renderer::Draw()` | **High** | 2 | ✅ **Resolved** — wrapped all 6 draw methods in `DeviceGL.cpp` and `DeviceVK.cpp` with `Profiler::BeginDrawCall`/`EndDrawCall` |
| 6 | `GF_BUILD_DEBUG_PROFILER` description misleading | **Low** | 0 | ✅ **Resolved** — option removed entirely; no conditional compilation |
| 7 | No `GF_PROFILE_SCOPE` in framework internals | **Medium** | 4 | ✅ **Resolved** — added to `ShaderGL::Init()`, `ShaderVK::Init()`, `UniformBufferGL::AddData()`, `UniformBufferVK::AddData()`, `TextureGL::Init()` (file + memory), `TextureVK::Init()` (file + memory) |
| 8 | Query pool overflow produces silent failures | **Medium** | 4 | ✅ **Resolved** — added `std::cerr` warning log in `GPUProfiler::BeginDrawCall()` when pool is full |
| 9 | Frame history not bounded (unbounded memory growth) | **Low** | 4 | ✅ **Resolved** — frame history vector capped at `MaxHistory` (120 frames) in `CPUProfiler::EndFrame()` |
| 10 | Missing profiler settings in sample `Settings.json` | **Low** | 4 | ✅ **Resolved** — added `Profiler` settings block to all 5 sample `Settings.json` files |
| 11 | `VK_TIME_DOMAIN_GPU_CLOCK_EXT` not available in Vulkan headers | **Medium** | 2 | ✅ **Resolved** — replaced with `VK_TIME_DOMAIN_QUERY_PERFORMANCE_COUNTER_EXT` |
| 12 | `VkPhysicalDeviceProperties::timestampPeriod` removed in Vulkan 1.3 | **Medium** | 2 | ✅ **Resolved** — replaced with `vkGetPhysicalDeviceQueueFamilyProperties` query; timestamps in nanoseconds via calibrated timestamps |
| 13 | `BeginDrawCall({}, ..., {})` brace init fails on MSVC | **High** | 2 | ✅ **Resolved** — replaced all `{}` with `""` empty string literals |
| 14 | `CommandBufferVK` type undefined in Vulkan builds | **High** | 2 | ✅ **Resolved** — added `#include "RHI/Vulkan/Internal/CommandBufferVK.h"` to `GPUProfiler.cpp` |
| 15 | No draw calls in the overlay (Vulkan) | **High** | 2 | ✅ **Resolved** — `vkCmdBeginQuery`/`vkCmdEndQuery` are invalid for `VK_QUERY_TYPE_TIMESTAMP`; replaced with `vkCmdWriteTimestamp` |
| 16 | `vkGetQueryPoolResults` always returned `VK_NOT_READY` | **High** | 2 | ✅ **Resolved** — the whole pool (`maxQueries * 2`) was queried, including unwritten slots. Now each draw's pair is read individually with `VK_QUERY_RESULT_WAIT_BIT` after waiting on the previous frame's fence |
| 17 | OpenGL build failed (`mQueryPool` referenced in an unguarded debug print) | **High** | 2 | ✅ **Resolved** — debug print removed; backend members are only referenced inside their `#ifdef` blocks |
| 18 | `GL_TIME_ELAPSED` results divided by 1e3 | **Medium** | 2 | ✅ **Resolved** — the query returns nanoseconds; divide by 1e6 |
| 19 | Frame-time graph only ever showed one bar | **Low** | 4 | ✅ **Resolved** — `CPUProfiler::StartFrame()` no longer resets `mSnapshot` (which wiped `frameTimeHistory`); the graph is now a bounded `ImGui::PlotHistogram` of the last 60 frames |
| 20 | Vulkan showed only ~20 of 260 draw calls, flickering | **High** | 2 | ✅ **Resolved** — the GPU timestamp counter has ~1 µs resolution, so short draws produce `begin == end`. Those pairs were discarded. Equal pairs are now kept (displayed as `<0.001ms`); only non-monotonic pairs are skipped. Frame GPU time is the span from first begin to last end |
| 21 | OpenGL showed "N draw calls" but no rows | **High** | 3 | ✅ **Resolved** — the CPU table used `height = -textLineHeight` and the draw list used `height = -30`, both of which collapse inside an auto-sized window. The Profiler window now has a default size, the CPU table a fixed 160 px scroll area and the draw list a fixed 240 px child |
| 22 | Shader/object names missing or inconsistent between backends | **Medium** | 3 | ✅ **Resolved** — `ShaderGL` never overrode `GetName()` (always empty) and `ShaderVK` returned its numeric ID (`"1"`). Both now return the vertex shader file name (`Drawing.vert`). Object names are read from an optional `"Name"` in the object JSON, fall back to the data-file stem (`Fish`, `Floor`, …), and are applied to draw calls via `Profiler::SetDrawCallObjectName()` from `ObjectManager::RenderObjects()` |
| 23 | Crash on exit in `GPUProfiler::CloseVulkan()` (`vkDestroyQueryPool`) | **High** | 4 | ✅ **Resolved** — `Engine::Stop()` destroyed the `VkDevice` (`Renderer::Close()`) before calling `Profiler::Close()`. The profiler is now closed first, and `CloseVulkan()` additionally guards `mDevice != VK_NULL_HANDLE` and nulls it |

---

## File map

| File | Responsibility |
|---|---|
| `Profiling/Profiler.h` | Public header — macros, `Profiler::Instance()` |
| `Profiling/Profiler.cpp` | Singleton lifecycle, frame boundary hooks |
| `Profiling/CPUProfiler.h` | `ScopeGuard` RAII class, `CPUProfiler` interface |
| `Profiling/CPUProfiler.cpp` | Sample collection, grouping/coalescing |
| `Profiling/GPUProfiler.h` | Query pool abstraction, draw call hooks |
| `Profiling/GPUProfiler.cpp` | Query pool management, backend-specific query execution |
| `Profiling/ProfilerOverlay.h` | ImGui visualization interface |
| `Profiling/ProfilerOverlay.cpp` | ImGui rendering, CSV export |
| `Profiling/ProfilerInternal.h` | Shared structs (`CPUSample`, `DrawCallSample`, `FrameBreakdown`) |

---

## Phase 0 — Foundation

**Goal:** Add profiler source files to the build; verified compilation in all configurations.

### 0.1 CMake changes

**File: `GraphicsFramework/CMakeLists.txt`** (root)

The `option(GF_BUILD_DEBUG_PROFILER ...)` line was **removed**. The profiler is now unconditionally part of the build.

**File: `GraphicsFramework/GraphicsFramework/Source/CMakeLists.txt`** (Source level)

- Removed the `target_compile_definitions` block that conditionally defined `GF_PROFILING` (it contained `if/else` logic that previously controlled Debug vs Release compilation).
- Removed the x64 library path workaround (`/LIBPATH` additions, `GF_X64_WINDOWS_LIBS` variable, `target_link_directories`).

**File: `GraphicsFramework/GraphicsFramework/Source/Core/Core.h`**

Removed `#define GF_PROFILING`. The compile definition is no longer used anywhere.

### 0.2 Create skeleton files

Create the following files (compile-only, no logic yet):
- `Source/Profiling/ProfilerInternal.h` — just struct declarations
- `Source/Profiling/CPUProfiler.h` — empty class declarations
- `Source/Profiling/CPUProfiler.cpp` — empty implementations
- `Source/Profiling/GPUProfiler.h` — empty class declarations
- `Source/Profiling/GPUProfiler.cpp` — empty implementations
- `Source/Profiling/Profiler.h` — stub macros
- `Source/Profiling/Profiler.cpp` — stub singleton
- `Source/Profiling/ProfilerOverlay.cpp` — stub `RenderOverlay`

### 0.3 Verify

```powershell
cmake --preset Ninja-Multi
cmake --build --preset Vulkan-Debug --target GraphicsFramework --parallel
cmake --build --preset Vulkan-Release --target GraphicsFramework --parallel
```

The profiler is now compiled and linked into **all** configurations (Debug and Release, both OpenGL and Vulkan).

---

## Phase 1 — CPU Profiler

**Goal:** RAII scope guard + macro API, sample collection with grouping, basic frame tracking.

### 1.1 `ProfilerInternal.h` — shared data structures

Define the core data types:
```cpp
struct CPUSample {
    const char*  name;
    double       avgMs;       // average time per call this frame
    double       totalMs;     // total time across all calls this frame
    int          callCount;   // how many times invoked this frame
};

struct FrameBreakdown {
    double gameLogicMs;
    double cpuRenderMs;
    double gpuTimeMs;
    double presentWaitMs;
    double totalFrameMs;
};

struct DrawCallSample {
    uint32_t   drawID;
    std::string objectName;
    std::string shaderName;
    std::string materialName;
    uint32_t   instanceCount;
    double     gpuTimeMs;
};
```

### 1.2 `CPUProfiler.h` / `CPUProfiler.cpp` — RAII guard + sample collection

**`ScopeGuard` class** (in `CPUProfiler.h`):
```cpp
class CPUProfiler::ScopeGuard {
public:
    explicit ScopeGuard(const char* name);
    ~ScopeGuard();  // captures end time, records sample
private:
    const char*  m_Name;
    uint64_t     m_StartTicks;
    uint64_t     m_EndTicks;
    bool         m_Recorded;
};
```

Implementation details:
- Constructor: captures `std::chrono::high_resolution_clock::now()`, stores ticks
- Destructor: captures end ticks, computes delta, calls `CPUProfiler::Instance()->RecordSample(name, ms)`
- `m_Recorded` flag ensures samples are only recorded once

**`CPUProfiler` class** (singleton):
```cpp
class CPUProfiler {
public:
    void RecordSample(const char* name, double milliseconds);
    void BeginFrame();
    const std::vector<CPUSample>& GetSamples() const;
private:
    struct RawSample {
        const char* name;
        double      milliseconds;
    };
    std::vector<RawSample> m_RawSamples;   // per-frame, not grouped
    std::vector<CPUSample> m_Samples;      // grouped, ready for overlay
};
```

### 1.3 `Profiler.h` — public macro API

```cpp
#include "Profiling/CPUProfiler.h"
#define GF_PROFILE_SCOPE(name) ::GraphicsFramework::Profiling::CPUProfiler::ScopeGuard _profScope(name)
#define GF_PROFILE_ONE_SHOT(name) do { auto _g = ::GraphicsFramework::Profiling::CPUProfiler::ScopeGuard(name); } while(0)
#define GF_PROFILE_BEGIN(name) auto _profScope_##__LINE__ = ::GraphicsFramework::Profiling::CPUProfiler::ScopeGuard(name)
#define GF_PROFILE_END() /* ScopeGuard destructor handles it */
```

No `#ifdef GF_PROFILING` — the profiler is always compiled, so the macros are always active.

### 1.4 `Profiler.cpp` — frame hooks

```cpp
void Profiler::OnFrameStart() {
    CPUProfiler::Instance()->BeginFrame();
    GPUProfiler::Instance()->BeginFrame();
}

void Profiler::OnFrameEnd() {
    // Group CPU samples
    // Flush GPU queries
    // Commit frame data
}
```

### 1.5 Integrate into Engine

**File: `Source/Core/Engine.cpp`**

Unconditional profiler hooks into the `Run()` loop:
```cpp
void Engine::Run()
{
    while (appIsRunning)
    {
        for (const NextFrameFunc& Func : StartFrameFuncs) { Func(); }
        StartFrameFuncs.clear();

        Time::Instance()->FrameStart();
        ImguiManager::Instance()->StartFrame();

        Profiler::Instance()->StartFrame();  // ← unconditional
        Profiler::Instance()->MarkRenderStart();

        Renderer::Instance()->StartFrame();
        Inputs::Instance()->Update();
        pCamera->Update();

        if (pScene) {
            pScene->DebugDisplay();
            pScene->Update();
        }

        Profiler::Instance()->MarkRenderEnd();

        ImguiManager::Instance()->Update();
        Renderer::Instance()->SwapBuffers();

        Profiler::Instance()->EndFrame();

        Time::Instance()->FrameEnd();

        for (const NextFrameFunc& Func : EndFrameFuncs) { Func(); }
        EndFrameFuncs.clear();
    }

    Renderer::Instance()->WaitFrameFinish();
    Profiler::Instance()->Close();  // ← unconditional
}
```

**File: `Source/Core/Engine.h`**

Unconditional `Profiler::Init()` call in `Engine::Init()`, settings parsing via `Profiler::ParseSettings()`, and `Profiler::Close()` in `Engine::Stop()`.

### 1.6 Verify

```powershell
cmake --build --preset Vulkan-Debug --target GraphicsFramework --parallel
cmake --build --preset Vulkan-Release --target GraphicsFramework --parallel
```

---

## Phase 2 — GPU Profiler

**Goal:** Per-draw-call GPU timing on both backends, query pool management.

### 2.1 `GPUProfiler.h` — interface

```cpp
class GPUProfiler {
public:
    void Init(uint32_t maxQueries = 256);
    void Close();

    void StartFrame();
    void EndFrame();

    // Called by Device to wrap each draw call
    void BeginDrawCall(uint32_t drawID, const char* objectName, const char* shaderName, const char* materialName, uint32_t instanceCount);
    void EndDrawCall();

    double GetFrameGPUTimeMs() const;
    const std::vector<DrawCallSample>& GetDrawCallSamples() const;
    bool HasCalibratedTimestamps() const;
};
```

### 2.2 Backend dispatch

**Vulkan** (`GPUProfiler.cpp` — `InitVulkan()`):
- Gets `VkDevice` and `VkPhysicalDevice` from `rhi::vk::Device::Get()`
- Queries `VK_EXT_calibrated_timestamps` support via `vkEnumerateDeviceExtensionProperties` (informational only)
- Creates `VkQueryPool` with `VK_QUERY_TYPE_TIMESTAMP` of size `maxQueries * 2` (begin + end)
- Reads `VkPhysicalDeviceLimits::timestampPeriod` (not `VkPhysicalDeviceProperties::timestampPeriod`, which does not exist)
- Each draw call is bracketed with `vkCmdWriteTimestamp(..., VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, ...)` — **not** `vkCmdBeginQuery`, which is invalid for timestamp pools
- Queries are allocated sequentially (`0..N-1`) and reset with `vkResetQueryPool` at the start of each frame
- The previous frame's fence is waited on, then **each draw's pair is read individually** with `vkGetQueryPoolResults(... VK_QUERY_RESULT_WAIT_BIT)`. Reading the whole pool returns `VK_NOT_READY` because unwritten slots are unavailable

**OpenGL** (`GPUProfiler.cpp` — `InitOpenGL()`):
- Uses `GL_ARB_timer_query` / `GL_EXT_timer_query`
- Query objects allocated sequentially per frame (pool resolved before reuse)
- `glBeginQuery(GL_TIME_ELAPSED, query)` / `glEndQuery` / `glGetQueryObjectui64v` (result is **nanoseconds**, divided by 1e6)

### 2.3 Draw call hooks

Draw call hooks are placed in the **RHI backend files**, not in the abstract `Renderer` class:

**File: `Source/RHI/OpenGL/DeviceGL.cpp`** — all 6 draw methods wrapped:
- `Device::Draw(Shader* shader, VertexBuffer* vb, IndexBuffer* ib)`
- `Device::Draw(Shader* shader, VertexBuffer* vb)`
- `Device::DebugDraw(Shader* shader, const std::vector<Vector2>& points, const Color& color)`
- `Device::DebugDrawLines(Shader* shader, const std::vector<Vector2>& points, const Color& color)`
- `Device::DrawDebugCircle(...)`
- `Device::DrawDebugLine(...)`

Each wrapped with `Profiler::Instance()->BeginDrawCall(...)` / `EndDrawCall()`, passing `shader->GetName()` as the shader name.

**File: `Source/RHI/Vulkan/DeviceVK.cpp`** — same 6 methods wrapped identically.

Shader name resolution:
- `Shader` base class has virtual `GetName()` returning empty string by default
- `ShaderGL::GetName()` returns `mName` (extracted from vertex shader file path in `Init()`)
- `ShaderVK::GetName()` returns `mShaderId`

**Files: `Source/Rendering/Renderer.h`** — forward-declares `class Profiler;`
**File: `Source/Rendering/Renderer.cpp`** — includes `Profiling/Profiler.h`

### 2.4 Settings integration

**File: `Source/Core/Engine.cpp`** — `Settings::Parse()` already includes profiler settings parsing (e.g. `MaxConcurrentQueries`).

### 2.5 Verify

```powershell
cmake --build --preset Vulkan-Debug --target GraphicsFramework --parallel
```

Run a sample scene and verify no crashes.

---

## Phase 3 — ImGui Overlay

**Goal:** Real-time profiler visualization in an ImGui window, toggleable via F11.

### 3.1 `ProfilerOverlay.cpp` — ImGui rendering

**Hotkey:** `F11` to toggle — checked in `ImguiManager::Update()`:
```cpp
if (ImGui::IsKeyPressed(ImGuiKey_F11))
{
    Profiler::Instance()->SetOverlayVisible(!Profiler::Instance()->IsOverlayVisible());
}
```

**Overlay layout:**

**GPU Panel:**
- Rolling bar chart of frame times (last 60 frames)
- `FPS: XXX.XX | Total: XXX.Xms`
- Breakdown: `CPU Render: X.Xms | GPU: X.Xms | Present: X.Xms`
- Draw call summary: `Calls: XXX | Instances: X,XXX`
- **Export CSV** button

**CPU Panel:**
- Sorted table (by total ms descending):
  | Function | Samples | Avg (ms) | Total (ms) |

**Draw call detail table:**
  | # | Object | Shader | Verts | Inst | GPU (ms) |

### 3.2 CSV Export

**Trigger:** When user clicks **Export CSV** button.

**Behavior:** Exports only the **previous frame's** data:
- Output file: `profiler_frame_NNN.csv` in working directory
- NNN = frame number (zero-padded, 3 digits)
- Format: CSV with two sections (CPU samples + draw call records)

Uses `std::ofstream` — no external dependencies.

### 3.3 Integrate into Engine loop

**File: `Source/Core/ImguiManager.cpp`**

- Unconditional `#include "Profiling/Profiler.h"`
- Unconditional overlay rendering call in `Update()`
- F11 hotkey toggle in `Update()` (using `ImGui::IsKeyPressed(ImGuiKey_F11)`)

### 3.4 Verify

```powershell
cmake --build --preset Vulkan-Debug --target UnitTests --parallel
```

Run a sample and verify:
- F11 shows profiler overlay
- Bars update each frame
- CSV export writes a file

---

## Phase 4 — Integration & Polish

**Goal:** Wrap up edge cases, add framework internal profiling, cleanup.

**Status: ✅ Complete**

### 4.1 Profile framework internals

Added `GF_PROFILE_SCOPE` calls in key framework hot paths:

**Shaders (file loading + compilation):**
- `Source/RHI/OpenGL/ShaderGL.cpp` — `GF_PROFILE_SCOPE` around shader file loading and compilation
- `Source/RHI/Vulkan/ShaderVK.cpp` — `GF_PROFILE_SCOPE` around shader SPIRV loading and pipeline creation

**Uniform buffers (data upload):**
- `Source/RHI/OpenGL/UniformBufferGL.cpp` — `GF_PROFILE_SCOPE` around `AddData()` upload
- `Source/RHI/Vulkan/UniformBufferVK.cpp` — `GF_PROFILE_SCOPE` around `AddData()` upload

**Textures (file loading + GPU upload):**
- `Source/RHI/OpenGL/TextureGL.cpp` — `GF_PROFILE_SCOPE` around file load + texture creation (both file-based and memory-based `Init()`)
- `Source/RHI/Vulkan/TextureVK.cpp` — `GF_PROFILE_SCOPE` around file load + staging + copy (both file-based and memory-based `Init()`)

### 4.2 Query pool overflow handling

When the draw call cap (256) is exceeded, `GPUProfiler::BeginDrawCall()` now logs a warning to `std::cerr`:
```
[GPUProfiler] Query pool full (256 queries). Dropping draw call profiling.
```

### 4.3 Ring buffer for frame history

Frame history is capped at `MaxHistory` (120 frames) in `CPUProfiler::EndFrame()`:
```cpp
if (mFrameHistory.size() >= FrameSnapshot::MaxHistory)
    mFrameHistory.erase(mFrameHistory.begin());
mFrameHistory.push_back(frameTimeMs);
```

### 4.4 Settings.json default

Updated all 5 sample `Settings.json` files with profiler settings:
- `FlockingParticles/res/Settings.json`
- `UnitTests/res/Settings.json`
- `AdvancedRenderingTechniques/res/Settings.json`
- `AnimationTechniques/res/Settings.json`
- `WaveSimulation/res/Settings.json`

### 4.5 Build verification

All 4 configurations build successfully:

| Preset | Result |
|---|---|
| `OpenGL-Debug` | ✅ |
| `OpenGL-Release` | ✅ |
| `Vulkan-Debug` | ✅ |
| `Vulkan-Release` | ✅ |

### 4.6 Vulkan header compatibility fixes

| Fix | Detail |
|---|---|
| `VK_TIME_DOMAIN_GPU_CLOCK_EXT` | Replaced with `VK_TIME_DOMAIN_QUERY_PERFORMANCE_COUNTER_EXT` (not available in current headers) |
| `VkPhysicalDeviceProperties::timestampPeriod` | Removed (deprecated in Vulkan 1.3); replaced with `vkGetPhysicalDeviceQueueFamilyProperties` query |
| `CommandBufferVK` undefined | Added `#include "RHI/Vulkan/Internal/CommandBufferVK.h"` to `GPUProfiler.cpp` |
| MSVC brace init | Replaced `BeginDrawCall({}, ..., {})` with `BeginDrawCall("", ..., "")` (MSVC doesn't accept `{}` for `const std::string&` parameters) |

---

## Implementation order summary

| Phase | What | Files touched |
|---|---|---|
| 0 | Skeleton + always-on CMake | `CMakeLists.txt` ×2, create `Profiling/*.h/.cpp` |
| 1 | CPU profiler | `Profiling/` (full), `Engine.h/cpp` |
| 2 | GPU profiler + draw hooks | `Profiling/` (full), `DeviceGL.cpp`, `DeviceVK.cpp` |
| 3 | ImGui overlay + F11 toggle | `Profiling/ProfilerOverlay.cpp`, `ImguiManager.cpp` |
| 4 | Polish | `Profiling/` (full), sample `Settings.json` files |

Total files: ~8 (`Profiling/Profiler.h/cpp`, `CPUProfiler.h/cpp`, `GPUProfiler.h/cpp`, `ProfilerOverlay.cpp`, `ProfilerInternal.h`)
Total modified files: ~5+ (`CMakeLists.txt` ×2, `Engine.h/cpp`, `DeviceGL.cpp`, `DeviceVK.cpp`, sample `Settings.json`)

---

## Risks & Mitigations

| Risk | Mitigation |
|---|---|
| Query pool exhaustion at 256 draws/frame | Flush oldest queries; log warning to console |
| Vulkan `VK_EXT_calibrated_timestamps` not available on some GPUs | Extension is checked at init; function pointers validated before use. Falls back to device clock domain |
| OpenGL `GL_ARB_timer_query` not available on some GPUs | Query for extension at init; gracefully disable GPU profiling if unavailable |
| Profiler overhead affecting frame times | `ScopeGuard` only stores raw ticks in destructor; grouping happens at frame boundary |
| Shader name resolution | Base `Shader::GetName()` returns empty string; `ShaderGL` extracts from file path; `ShaderVK` uses shader ID |

---

## Technical notes

### Always-on design

The profiler was originally designed with a `GF_BUILD_DEBUG_PROFILER` CMake option and `GF_PROFILING` compile definitions for conditional compilation. This was removed in favor of an always-on approach:

- **No `GF_BUILD_DEBUG_PROFILER`** — the CMake option no longer exists in any `CMakeLists.txt`.
- **No `GF_PROFILING` compile definition** — removed from `Source/CMakeLists.txt`.
- **No `#ifdef GF_PROFILING`** — all profiler code is unconditionally compiled and linked.
- **No `#define GF_PROFILING`** — removed from `Core.h`.

This simplifies the codebase (no conditional compilation scattered throughout) at the cost of slightly larger binary size in Release builds. The profiler's overhead in the absence of draw call hooks is minimal (frame timing is just `std::chrono` calls).

### Draw call hook placement

Draw call hooks are in `DeviceGL.cpp` and `DeviceVK.cpp` (the concrete RHI implementations), not in the abstract `Renderer` class. This is because `Renderer` is an abstract base with no access to `Shader*` or buffer pointers — those are only available in the backend-specific `Device` classes where the actual GL/Vulkan draw commands are issued.

### Vulkan calibrated timestamps

`VK_EXT_calibrated_timestamps` is queried at init time via `vkEnumerateDeviceExtensionProperties`. If available, the extension's `vkGetCalibratedTimestampsEXT` and `vkGetPhysicalDeviceCalibrateableTimeDomainsEXT` function pointers are loaded via `vkGetDeviceProcAddr`. The GPU clock domain (`VK_TIME_DOMAIN_GPU_CLOCK_EXT`) is preferred for the most accurate timestamps. If unavailable, the fallback is the device clock domain (`VK_TIME_DOMAIN_DEVICE_EXT`).
