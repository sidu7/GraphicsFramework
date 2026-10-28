# GraphicsFramework Profiler — Implementation Plan

**Goal:** Add a CPU + GPU profiler to GraphicsFramework that both framework internals and user scenes can use. It provides RAII scope guards for CPU profiling, per-draw-call GPU timing (Vulkan + OpenGL), an ImGui overlay for real-time inspection, and CSV export.

**Constraints:**
- Windows x64 + MSVC only
- Always compiled in Debug, off by default in Release (`GF_BUILD_DEBUG_PROFILER` CMake flag)
- Zero overhead when `GF_PROFILING` is not defined (entire `Profiling/` excluded from build)
- Follows framework conventions: `Singleton` pattern, ImGui overlay, `rapidjson` settings

---

## File map

| File | Responsibility |
|---|---|
| `Source/Profiling/Profiler.h` | Public header — macros, `Profiler::Instance()`, `#ifdef GF_PROFILING` guards |
| `Source/Profiling/Profiler.cpp` | Singleton lifecycle, frame boundary hooks |
| `Source/Profiling/CPUProfiler.h` | `ScopeGuard` RAII class, `CPUProfiler` interface |
| `Source/Profiling/CPUProfiler.cpp` | Sample collection, grouping/coalescing |
| `Source/Profiling/GPUProfiler.h` | Query pool abstraction, draw call hooks |
| `Source/Profiling/GPUProfiler.cpp` | Query pool management, backend-specific query execution |
| `Source/Profiling/ProfilerOverlay.cpp` | ImGui visualization, hotkey handling, CSV export |
| `Source/Profiling/ProfilerInternal.h` | Shared structs (`CPUSample`, `DrawCallRecord`, `FrameBreakdown`) |

All profiler files are excluded from the release build when `GF_BUILD_DEBUG_PROFILER` is OFF (controlled in `Source/CMakeLists.txt`).

---

## Phase 0 — Foundation

**Goal:** Add profiler source files to the build, verify compilation in Debug, confirm zero linkage in Release.

### 0.1 CMake changes

**File: `GraphicsFramework/CMakeLists.txt`** (root)

Add CMake option:
```cmake
option(GF_BUILD_DEBUG_PROFILER "Enable profiler in debug builds" ON)
```

**File: `GraphicsFramework/GraphicsFramework/Source/CMakeLists.txt`** (Source level)

Add conditional compilation definition:
```cmake
# Add after existing target_compile_definitions
target_compile_definitions(GraphicsFramework PRIVATE
    "$<$<AND:$<OR:$<CONFIG:Debug_OpenGL>,$<CONFIG:Release_OpenGL>>,$<BOOL:${GF_BUILD_DEBUG_PROFILER}>>:GF_PROFILING>"
    "$<$<AND:$<OR:$<CONFIG:Debug_Vulkan>,$<CONFIG:Release_Vulkan>>,$<BOOL:${GF_BUILD_DEBUG_PROFILER}>>:GF_PROFILING>"
)
```

Add `Profiling/` to common sources via `file(GLOB_RECURSE ...)` — already picked up since the glob is `"${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"`.

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
cmake --build --preset OpenGL-Debug --target GraphicsFramework --parallel
cmake --build --preset Vulkan-Debug --target GraphicsFramework --parallel
cmake --build --preset OpenGL-Release --target GraphicsFramework --parallel
cmake --build --preset Vulkan-Release --target GraphicsFramework --parallel
```

Check symbol counts in debug vs release:
```powershell
dumpbin /symbols build\ninja-multi\GraphicsFramework_Binary\Debug_OpenGL\GraphicsFramework.lib | sls -i "profiler"   # should have entries
dumpbin /symbols build\ninja-multi\GraphicsFramework_Binary\Release_OpenGL\GraphicsFramework.lib | sls -i "profiler"  # should have ZERO entries
```

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

struct DrawCallRecord {
    uint32_t   drawID;
    const char* objectName;
    const char* shaderName;
    const char* materialName;
    uint32_t   vertexCount;
    uint32_t   indexCount;
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
- `m_Recorded` flag ensures samples are only recorded once (destructor always runs, but guard for safety)

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

- `RecordSample()`: appends to `m_RawSamples` (thread-safe via lock, though single-threaded is fine for now)
- `BeginFrame()`: clears `m_RawSamples`, swaps `m_Samples` for reading
- `GetSamples()`: **groups** samples by name into `CPUSample` entries (count, avg, total)

Grouping logic (runs at frame boundary):
```
for each RawSample:
    if name exists in groups:
        groups[name].callCount++
        groups[name].totalMs += milliseconds
    else:
        create new group
after grouping:
    for each group:
        avgMs = totalMs / callCount
```

### 1.3 `Profiler.h` — public macro API

```cpp
#ifdef GF_PROFILING
    #include "Profiling/CPUProfiler.h"
    #define GF_PROFILE_SCOPE(name) ::GraphicsFramework::Profiling::CPUProfiler::ScopeGuard _profScope(name)
    #define GF_PROFILE_ONE_SHOT(name) do { auto _g = ::GraphicsFramework::Profiling::CPUProfiler::ScopeGuard(name); } while(0)
    #define GF_PROFILE_BEGIN(name) auto _profScope_##__LINE__ = ::GraphicsFramework::Profiling::CPUProfiler::ScopeGuard(name)
    #define GF_PROFILE_END() /* ScopeGuard destructor handles it */
#else
    #define GF_PROFILE_SCOPE(name)
    #define GF_PROFILE_ONE_SHOT(name)
    #define GF_PROFILE_BEGIN(name)
    #define GF_PROFILE_END()
#endif
```

**Important naming convention:** prefix with `::GraphicsFramework::` namespace to avoid macro collisions.

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

Add profiler hooks into the `Run()` loop:

```cpp
void Engine::Run()
{
    while (appIsRunning)
    {
        for (const NextFrameFunc& Func : StartFrameFuncs) { Func(); }
        StartFrameFuncs.clear();

        Time::Instance()->FrameStart();
        ImguiManager::Instance()->StartFrame();

#ifdef GF_PROFILING
        Profiler::Instance()->OnFrameStart();
        m_FrameLogicStart = std::chrono::high_resolution_clock::now();
#endif

        Renderer::Instance()->StartFrame();
        Inputs::Instance()->Update();
        pCamera->Update();

#if !NO_IMGUI
        for (CustomDebugFunc& Func : CustomDebugFunctions) { Func(); }
#endif

        if (pScene) {
            pScene->DebugDisplay();
            pScene->Update();
        }

#ifdef GF_PROFILING
        m_FrameGameLogicEnd = std::chrono::high_resolution_clock::now();
#endif

        ImguiManager::Instance()->Update();

#ifdef GF_PROFILING
        m_FrameRenderStart = std::chrono::high_resolution_clock::now();
#endif

        Renderer::Instance()->SwapBuffers();

#ifdef GF_PROFILING
        m_FramePresentStart = std::chrono::high_resolution_clock::now();
#endif

#ifdef GF_PROFILING
        m_FramePresentEnd = std::chrono::high_resolution_clock::now();
        // Compute breakdown
        Profiler::Instance()->CommitFrame(...);
#endif

        Time::Instance()->FrameEnd();

        for (const NextFrameFunc& Func : EndFrameFuncs) { Func(); }
        EndFrameFuncs.clear();
    }

    Renderer::Instance()->WaitFrameFinish();

#ifdef GF_PROFILING
    Profiler::Instance()->Shutdown();
#endif
}
```

Add member variables to `Engine.h`:
```cpp
#ifdef GF_PROFILING
    std::chrono::high_resolution_clock::time_point m_FrameLogicStart;
    std::chrono::high_resolution_clock::time_point m_FrameGameLogicEnd;
    std::chrono::high_resolution_clock::time_point m_FrameRenderStart;
    std::chrono::high_resolution_clock::time_point m_FramePresentStart;
    std::chrono::high_resolution_clock::time_point m_FramePresentEnd;
#endif
```

### 1.6 Verify

```powershell
cmake --build --preset OpenGL-Debug --target GraphicsFramework --parallel
cmake --build --preset Vulkan-Debug --target GraphicsFramework --parallel
```

---

## Phase 2 — GPU Profiler

**Goal:** Per-draw-call GPU timing on both backends, query pool management.

### 2.1 `GPUProfiler.h` — interface

```cpp
class GPUProfiler {
public:
    void Initialize();
    void Shutdown();

    void BeginFrame();
    void EndFrame();

    // Called by Renderer to wrap each draw call
    void RecordDrawCallBegin(uint32_t drawID, const char* objectName, const char* shaderName, const char* materialName);
    void RecordDrawCallEnd(uint32_t drawID);

    uint32_t GetMaxConcurrentQueries() const { return m_MaxConcurrentQueries; }

    const std::vector<DrawCallRecord>& GetDrawRecords() const { return m_DrawRecords; }
    double GetTotalGPUTime() const;

private:
    // Backend-specific query operations
    void BackendFlushQueries();
    void BackendRecordBegin(uint32_t queryIndex);
    void BackendRecordEnd(uint32_t queryIndex);
    double BackendReadQueryTime(uint32_t queryIndex);
    uint32_t BackendAcquireQuery();

    uint32_t m_MaxConcurrentQueries;
    std::vector<DrawCallRecord> m_DrawRecords;
    // Query pool state — backend-specific handles stored in GPUProfilerImpl
    void* m_Impl;
};
```

### 2.2 Backend dispatch

**Vulkan** (`Source/RHI/Vulkan/GPUProfilerVK.cpp`):
- Uses `VK_QUERY_TYPE_TIMESTAMP` for GPU timestamps
- Query pool size = `m_MaxConcurrentQueries * 2` (begin + end)
- `VK_EXT_calibrated_timestamps` for CPU↔GPU time correlation
- Records timestamps in the command buffer; reads results after `vkQueueSubmit`

**OpenGL** (`Source/RHI/OpenGL/GPUProfilerGL.cpp`):
- Uses `GL_ARB_timer_query`
- Query objects stored in a ring buffer
- `glBeginQuery(GL_TIME_ELAPSED, query)` / `glEndQuery` / `glGetQueryObjectui64v`

The query pool is managed inside the RHI backend files — the `GPUProfiler` interface is backend-agnostic.

### 2.3 Integrate draw call hooks into Renderer

**File: `Source/Rendering/Renderer.h`** — add forward declaration:
```cpp
class GPUProfiler;
```

**File: `Source/Rendering/Renderer.cpp`** — wrap draw calls:

```cpp
// In Renderer::Draw(const Shader* shader, ...) — OpenGL draw:
#ifdef GF_PROFILING
    auto* gpuProf = GPUProfiler::Instance();
    // Get object name from currently drawn object (from ObjectManager or renderer state)
    gpuProf->RecordDrawCallBegin(drawID, objectName.c_str(), shaderName, materialName);
#endif
    // ... actual backend draw call ...
#ifdef GF_PROFILING
    gpuProf->RecordDrawCallEnd(drawID);
#endif

// Same for all Draw() overloads.
```

Since draw calls go through `Renderer::Draw()` and `Renderer::DebugDraw()` etc., all draws are captured. The `ObjectManager` provides the current object being drawn.

### 2.4 Settings integration

**File: `Source/Core/Engine.cpp`** — `Settings::Parse()`:

```cpp
void Settings::Parse(rapidjson::Document& settingsFile)
{
    // ... existing parsing ...

    if (Root.HasMember("Profiler"))
    {
        auto& prof = Root["Profiler"];
        if (prof.HasMember("MaxConcurrentQueries"))
        {
            m_MaxConcurrentQueries = prof["MaxConcurrentQueries"].GetUint();
        }
    }
}
```

**File: `Source/Core/Engine.h`** — add to `Settings` struct:
```cpp
uint32_t m_MaxConcurrentQueries;
```

### 2.5 Verify

```powershell
cmake --build --preset OpenGL-Debug --target GraphicsFramework --parallel
cmake --build --preset Vulkan-Debug --target GraphicsFramework --parallel
cmake --build --preset OpenGL-Release --target GraphicsFramework --parallel
cmake --build --preset Vulkan-Release --target GraphicsFramework --parallel
```

Run a sample scene and verify no crashes.

---

## Phase 3 — ImGui Overlay

**Goal:** Real-time profiler visualization in an ImGui window, toggleable via hotkey.

### 3.1 `ProfilerOverlay.cpp` — ImGui rendering

**Hotkey:** `F11` to toggle. Checked in `ImguiManager::Update()` — call `Profiler::Instance()->RenderOverlay()` after ImGui update.

**Overlay layout** (two panels using ImGui columns or nested windows):

**GPU Panel:**
- Rolling bar chart of frame times (last 60 frames) — use `ImGui::PlotLines` or `ImGui::PlotHistogram`
- `FPS: XXX.XX | Total: XXX.Xms`
- Breakdown: `CPU Render: X.Xms | GPU: X.Xms | Present: X.Xms`
- Draw call summary: `Calls: XXX | Instances: X,XXX`
- **Export CSV** button

**CPU Panel:**
- Sorted table (by total ms descending):
  | Function | Samples | Avg (ms) | Total (ms) |
  |---|---|---|---|
  | `Physics::Step()` | 60 | 0.80 | 48.0 |

- **Draw call detail table:**
  | # | Object | Shader | Verts | Idx | Inst | GPU (ms) |
  |---|---|---|---|---|---|---|
  | 1 | `Tree_001` | `tree.frag` | 128 | 72 | 1 | 0.032 |

### 3.2 CSV Export

**Trigger:** When user clicks **Export CSV** button.

**Behavior:** Exports only the **previous frame's** data:
- Output file: `profiler_frame_NNN.csv` in working directory
- NNN = frame number (zero-padded, 3 digits)
- Format: CSV with two sections (CPU samples + draw call records)

```csv
# Frame NNN — exported at frame M
# FPS: 59.88 | Total: 16.7ms | CPU Render: 4.2ms | GPU: 8.1ms | Present: 4.4ms

# CPU Samples
Function,Samples,Avg_ms,Total_ms
Physics::Step(),60,0.80,48.0
Scene::Update(),60,2.10,126.0

# Draw Calls
DrawID,Object,Shader,Verts,Idx,Inst,GPU_ms
0,Tree_001,tree.frag,128,72,1,0.032
1,Ground,terrain.frag,4096,3840,1,0.041
```

Uses `std::ofstream` — no external dependencies.

### 3.3 Integrate into Engine loop

**File: `Source/Core/ImguiManager.cpp`** or `Source/Core/Engine.cpp`:

```cpp
// In Engine::Run(), after ImguiManager::Update():
#ifdef GF_PROFILING
    if (Inputs::Instance()->IsKeyPressed(SDL_SCANCODE_F11))
    {
        Profiler::Instance()->ToggleOverlay();
    }
    Profiler::Instance()->RenderOverlay();
#endif
```

### 3.4 Verify

```powershell
cmake --build --preset Vulkan-Debug --target UnitTests --parallel
# Run a sample and verify:
# - F11 shows profiler overlay
# - Bars update each frame
# - CSV export writes a file
```

---

## Phase 4 — Integration & Polish

**Goal:** Wrap up edge cases, add framework internal profiling, cleanup.

### 4.1 Profile framework internals

Add `GF_PROFILE_SCOPE` calls in key framework hot paths:
- `Renderer::Draw()` — wrap each backend draw
- `Shader::Init()` — shader compilation time
- `UniformBuffer::AddData()` — UBO upload time
- `Texture::Init()` — texture upload time

### 4.2 Query pool overflow handling

When the draw call cap (256) is exceeded:
- Flush oldest queries (read results from GPU)
- Reuse query slots
- Ensure no query is read twice

### 4.3 Ring buffer for frame history

Store last N frames (configurable, default 128) in a ring buffer for the overlay graph:
```cpp
constexpr int RING_BUFFER_SIZE = 128;
FrameBreakdown m_FrameHistory[RING_BUFFER_SIZE];
int m_FrameHead = 0;
```

### 4.4 Settings.json default

Update sample `Settings.json` files with profiler default:
```json
{
    "WindowSize": [ 1280.0, 720.0 ],
    "TransformBindingPoint": 0,
    "MaterialBindingPoint": 1,
    "Profiler": {
        "MaxConcurrentQueries": 256
    }
}
```

### 4.5 Verify regression

```powershell
# All presets
cmake --build --preset OpenGL-Debug --target GraphicsFramework --parallel
cmake --build --preset OpenGL-Release --target GraphicsFramework --parallel
cmake --build --preset Vulkan-Debug --target GraphicsFramework --parallel
cmake --build --preset Vulkan-Release --target GraphicsFramework --parallel

# Samples
cmake --build --preset Vulkan-Debug --target FlockingParticles --parallel
cmake --build --preset Vulkan-Debug --target UnitTests --parallel

# Visual verification
scripts\verify-scene.ps1 -Preset Vulkan-Debug -Scene 0
```

---

## Implementation order summary

| Phase | What | Files touched |
|---|---|---|
| 0 | Skeleton + CMake | `CMakeLists.txt` ×2, create `Profiling/*.h/.cpp` |
| 1 | CPU profiler | `Profiling/` (full), `Engine.h/cpp` |
| 2 | GPU profiler | `Profiling/` (full), `Renderer.cpp`, `Settings::Parse()` |
| 3 | ImGui overlay | `Profiling/ProfilerOverlay.cpp`, `Engine.cpp` |
| 4 | Polish | `Profiling/` (full), sample `Settings.json` files |

Total new files: ~8 (`Profiling/Profiler.h/cpp`, `CPUProfiler.h/cpp`, `GPUProfiler.h/cpp`, `ProfilerOverlay.cpp`, `ProfilerInternal.h`)
Total modified files: ~5 (`CMakeLists.txt` ×2, `Engine.h/cpp`, `Renderer.cpp`, sample `Settings.json`)

---

## Risks & Mitigations

| Risk | Mitigation |
|---|---|
| Query pool exhaustion at 256 draws/frame | Flush oldest queries; log warning to console |
| Vulkan timestamp query requires `VK_EXT_calibrated_timestamps` extension | Check feature at init; fall back to simple `VK_QUERY_TYPE_TIMESTAMP` without calibration |
| OpenGL `GL_ARB_timer_query` not available on all GPUs | Query for extension at init; gracefully disable GPU profiling if unavailable |
| Profiler overhead affecting frame times significantly | `ScopeGuard` only stores raw ticks in destructor; grouping happens at frame boundary, not during hot path |
| Object name resolution during draw call | Store current object pointer in renderer state; resolve name at draw call time |
