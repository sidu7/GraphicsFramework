# RHI Migration Plan — OpenGL / Vulkan

Status: **in progress** — Phase 0, 1, 1.5, 2 and 3 complete.
Last updated: 2026-10-26.

This document is the source of truth for the rendering-abstraction refactor. It describes the
current problems, the target architecture, the file layout, and the ordered migration phases.
Each phase must leave the repository buildable (see [Acceptance](#8-acceptance-criteria)).

---

## 1. Motivation

1. `RenderingFactorySelector` exists only to be a template argument for
   `SingletonByFactory<T, Factory>`. The backend `#if` switch is duplicated in three files
   (`RenderingFactorySelector.cpp`, `Renderer.cpp`, `Core/Window.cpp`), and an unknown config
   falls off the end of a non-void function (undefined behaviour instead of a build error).
2. `RenderingFactory` mixes two unrelated responsibilities: *creating abstract resource objects*
   and *Vulkan device-memory helpers* (`CreateBuffer`, `CreateImage`, `FindMemoryType`). Because of
   that, every Vulkan resource file downcasts a global:
   `static_cast<RenderingFactory_Vulkan*>(RenderingFactory::Instance())`.
3. `RenderingFactory_Vulkan.h` also declares `BufferInfo` / `ImageInfo` and includes
   `Internal/DescriptorSet_Vulkan.h`, so other Vulkan headers include the factory header just to
   get those structs. The include graph is inverted.
4. Two globals (`Renderer::Instance()`, `RenderingFactory::Instance()`) have parallel lifetimes and
   are both backend-switched. In Vulkan the renderer *is* the device (it owns `VkDevice`,
   `VkPhysicalDevice`, `MemoryProperties`).
5. Public interface headers leak backend dependencies: `Rendering/Texture.h` and
   `IndexBuffer.h` include `Rendering/Renderer.h`, which includes GLEW and SDL. Shared enums
   (`CullFace`, `Rect2D/3D`, `ImageFormat`, `VertexFormat`, `IndexType`) have no single home.
6. Creation returns raw `new` pointers with no owner. Several paths leak or allocate inside draw
   calls.
7. Backend differences are expressed as scattered `#ifdef`s in framework code
   (`Camera.cpp` Y-flip, `ImguiManager.cpp` downcasts), not as RHI capabilities.

---

## 2. Current state at the start of the migration

| Area | File(s) | Note |
|---|---|---|
| Backend selection | `Rendering/RenderingFactorySelector.cpp`, `Rendering/Renderer.cpp`, `Core/Window.cpp` | three duplicated `#if` chains |
| Abstract factory | `Rendering/RenderingFactory.h` | 8 `Create*` + `GetApiName` |
| GL factory | `Rendering/OpenGL/RenderingFactory_OpenGL.{h,cpp}` | 8 one-line `new` methods |
| VK factory | `Rendering/Vulkan/RenderingFactory_Vulkan.{h,cpp}` | 8 `new` methods **plus** `CreateBuffer`, `CreateImage`, `FindMemoryType`, `BufferInfo`, `ImageInfo` |
| Device API | `Rendering/Renderer.h` | single class: frame lifecycle + bind/draw + no resource creation |
| Backend types | `Rendering/OpenGL/*_OpenGL.*`, `Rendering/Vulkan/*_Vulkan.*` | suffix naming, backend headers included from common code |

`Create*` call sites (before phase 1): **98** across the framework and samples.
`static_cast<RenderingFactory_Vulkan*>` sites: **6**.

### Known baseline breakages (pre-existing, not caused by this plan)

* `Vulkan-Debug` did not build: the `imgui` third-party target did not link the
  `vulkan_headers` interface target, so `imgui_impl_vulkan.h` could not find `vulkan/vulkan.h`.
  **Fixed** as part of Phase 0/1 verification (`ThirdParty/CMakeLists.txt`) and folded into
  [Phase 3](#phase-3--cmake-backend-isolation).
* `Rendering/Vulkan/ComputeShader_Vulkan.cpp` did not compile (pre-existing, previously masked by
  the `imgui` failure): header/implementation signature mismatch for `LoadShaderModule`,
  `SpvReflectDescriptorBinding**` assigned to `const VkDescriptorSetLayoutBinding*`, and calls to
  `Renderer_Vulkan::BeginSingleTimeCommands` / `EndSingleTimeCommands`, which did not exist.
  **Fixed** in Phase 1.5.
* `IndexBuffer_Vulkan.cpp` called `RenderingFactory_Vulkan::GetDevice()`, which does not exist.
  Resolved by Phase 1 (the call sites now use `Renderer_Vulkan::Get()`).
* Several sample targets reference APIs that were already removed before this plan
  (`CreateVertexArray`, `CreateElementArrayBuffer`, `RayTracing.cpp: DrawAllObjects`). They do not
  compile today. Phase 1 only performs the mechanical factory rename in them; reviving or deleting
  those samples is a separate decision.

---

## 3. Target architecture

Three layers, one selection seam, and the decision made by CMake rather than by `#ifdef`.

```
Core / Engine / Scene          no GL or VK headers
        |
Rendering (framework)          Mesh, Model, ShaderRegistry, Renderer facade
        |
RHI (abstract)                 Device, Texture, Shader, ...  (no GL/VK/SDL/GLEW)
        |
   +----+----+
   |         |
OpenGL     Vulkan              exactly one backend is compiled into a config
```

### 3.1 Backend selection

Declared once, defined once per backend, chosen by the linker:

```cpp
// Source/RHI/Backend.h  (public, backend-agnostic)
namespace rhi {
    std::unique_ptr<Device> CreateDevice();
}
```

```cpp
// Source/RHI/OpenGL/BackendGL.cpp
namespace rhi { std::unique_ptr<Device> CreateDevice() { return std::make_unique<gl::DeviceGL>(); } }

// Source/RHI/Vulkan/BackendVK.cpp
namespace rhi { std::unique_ptr<Device> CreateDevice() { return std::make_unique<vk::DeviceVK>(); } }
```

Because CMake compiles exactly one backend folder per configuration, exactly one definition of
`CreateDevice` is ever linked. No selector class, no `#if` chain, and misconfiguration becomes a
**linker error** instead of silent UB.

Runtime backend registries (`REGISTER_BACKEND(...)`) are explicitly **not** used: they would
require both backends in every binary, both driver stacks, and both shader formats, for no gain
while the RHI is still not fully API-agnostic.

### 3.2 Device owns creation

```cpp
class Device
{
public:
    virtual ~Device() = default;
    virtual const char* GetApiName() const = 0;

    virtual void BeginFrame() = 0;
    virtual void EndFrame() = 0;
    virtual void WaitIdle() = 0;

    virtual std::unique_ptr<Shader>              CreateShader() = 0;
    virtual std::unique_ptr<ComputeShader>       CreateComputeShader() = 0;
    virtual std::unique_ptr<Texture>             CreateTexture() = 0;
    virtual std::unique_ptr<FrameBuffer>         CreateFrameBuffer() = 0;
    virtual std::unique_ptr<VertexBuffer>        CreateVertexBuffer() = 0;
    virtual std::unique_ptr<IndexBuffer>         CreateIndexBuffer() = 0;
    virtual std::unique_ptr<UniformBuffer>       CreateUniformBuffer() = 0;
    virtual std::unique_ptr<ShaderStorageBuffer> CreateShaderStorageBuffer() = 0;

    // explicit render-pass scope (Vulkan cannot switch targets mid-pass)
    virtual void BeginRenderPass(const FrameBuffer* target) = 0;
    virtual void EndRenderPass() = 0;
    // ... bind/draw/state ...
};
```

Resource creation returns `std::unique_ptr`, so ownership is explicit and leaks become compile
errors.

---

## 4. Target file layout

Achieved after Phase 4:

```
Source/
├── RHI/                               # backend seam + implementation
│   ├── Backend.h                      # CreateDevice / CreateWindow declarations
│   ├── Types.h                        # CullFace, Rect2D/3D, ImageFormat, VertexFormat, IndexType
│   │
│   ├── OpenGL/                        # namespace rhi::gl
│   │   ├── BackendGL.cpp              # defines rhi::CreateDevice / rhi::CreateWindow
│   │   ├── DeviceGL.{h,cpp}           # class rhi::gl::Device : public Renderer
│   │   ├── TextureGL.{h,cpp}  ShaderGL.{h,cpp}  FrameBufferGL.{h,cpp}  ...
│   │   ├── (GLCall / GLClearError / GLLogCall live at the top of DeviceGL.h)
│   │   └── WindowGL.{h,cpp}
│   │
│   └── Vulkan/                        # namespace rhi::vk
│       ├── BackendVK.cpp
│       ├── DeviceVK.{h,cpp}           # class rhi::vk::Device : public Renderer
│       ├── VulkanTypes.h              # BufferInfo / ImageInfo / handles
│       ├── TextureVK.{h,cpp}  ShaderVK.{h,cpp}  FrameBufferVK.{h,cpp}  ...
│       ├── WindowVK.{h,cpp}
│       └── Internal/                  # HelperVK (layer/extension/device setup), swapchain,
│                                      # render pass, command buffers, queues, descriptors, fences
│
├── Rendering/                         # backend-agnostic layer (location unchanged)
│   ├── Renderer.{h,cpp}               # abstract device + DrawQuad/debug draw helpers
│   ├── {Texture,Shader,ComputeShader,FrameBuffer,VertexBuffer,
│   │    IndexBuffer,UniformBuffer,ShaderStorageBuffer}.h
│   └── ShaderRegistry.{h,cpp}
│
├── Assets/                            # Mesh, Model (assimp) — not RHI
├── Utils/Math/Quaternion.{h,cpp}      # pure math
└── Core/                              # Engine, Window, Camera, Scene, Components
```

Deviation from the original target: the abstract resource interfaces were **not** moved to
`RHI/Resources/*`. They stay in `Rendering/`, next to `Renderer.h`, because that directory is
already the backend-agnostic layer and moving them would only churn include paths without changing
what the build isolates (the Phase 3 CMake split already guarantees a configuration compiles exactly
one backend). `RHI/` therefore contains the seam (`Backend.h`, `Types.h`) plus the two backend
implementations; `Renderer.h` remains the abstract device interface.

Naming: the `_OpenGL` / `_Vulkan` suffixes were replaced by namespaces (`rhi::gl`, `rhi::vk`) plus a
shorter, uniform `GL` / `VK` suffix (`Renderer_OpenGL` → `rhi::gl::Device`, `Texture_Vulkan` →
`rhi::vk::TextureVK`). The suffix is deliberately **kept**: naming a class `rhi::gl::Texture` that
derives from `::Texture` makes the derived name hide the base name inside the namespace, so the base
class would no longer be found when matching the `override`s and the compiler would silently treat
them as new virtuals. The `GL`/`VK` suffix keeps every signature unambiguous while the namespace
already communicates the backend at call sites.

---

## 5. Phases

| Phase | Title | Main outcome | Risk |
|---|---|---|---|
| 0 | Header hygiene | shared types live in `RHI/Types.h`; interface headers stop pulling GLEW/SDL | low |
| 1 | Remove factory/selector | one global (`Renderer`), creation folded into it | low |
| 1.5 | Repair Vulkan compute shader | `ComputeShader_Vulkan.cpp` compiles and submits dispatches | low |
| 2 | Backend seam | `RHI/Backend.h` + per-backend `CreateDevice`/`CreateWindow`; no selector `#if` | medium |
| 3 | CMake backend isolation | backend source sets become real targets; only the matching backend is compiled and linked | medium |
| 4 | Folder restructure + rename | `RHI/{OpenGL,Vulkan}`, `Assets/`, `Utils/Math/`; namespace renames | medium |
| 5 | Vulkan device helpers + VMA | `CreateBuffer`/`CreateImage` move to `DeviceVK`; hand-rolled allocation replaced by VMA | medium |
| 6 | Backend capability seam | ImGui/Window hooks, `GetClipSpace()` replaces Camera `#if` | low |
| 7 | Pipeline objects (optional) | real pipeline + render-pass objects; no render pass baked into `Shader::Init` | high |

### Phase 0 — Header hygiene

* Add `Source/RHI/Types.h` with `CullFace`, `Rect2D`, `Rect3D` (moved out of `Renderer.h`).
* `Renderer.h` no longer includes GLEW or SDL; backend headers include what they need
  (`Renderer_OpenGL.h` now includes GLEW itself).
* `IndexBuffer.h` / `Texture.h` / `VertexBuffer.h` include only the standard headers they use
  instead of `Rendering/Renderer.h` / `Core/Core.h`.
* Add `virtual ~Renderer()`.
* `GetApiName()` returns `const char*` instead of `std::string`.

Exit criteria: `Renderer.h` compiles without GLEW/SDL on the include path; OpenGL and Vulkan
framework builds are unchanged.

### Phase 1 — Remove `RenderingFactory` and `RenderingFactorySelector`

* Delete `Rendering/RenderingFactory.h`, `Rendering/RenderingFactorySelector.cpp`,
  `Rendering/OpenGL/RenderingFactory_OpenGL.*`, `Rendering/Vulkan/RenderingFactory_Vulkan.*`.
* Move the 8 `Create*` methods and `GetApiName()` onto `Renderer` (pure virtual) and implement
  them in `Renderer_OpenGL` / `Renderer_Vulkan`.
* `Renderer` no longer inherits `SingletonByFactory`; `Renderer::Instance()` is implemented
  explicitly in `Renderer.cpp` (the `#if` chain disappears completely in Phase 2).
* Vulkan-only helpers (`FindMemoryType`, `CreateBuffer`, `CreateImage`) move onto `Renderer_Vulkan`
  as the natural device object; `BufferInfo`/`ImageInfo` move to `Rendering/Vulkan/VulkanTypes.h`.
* Rewrite all call sites: `RenderingFactory::Instance()->CreateX()` →
  `Renderer::Instance()->CreateX()`; includes `Rendering/RenderingFactory.h` →
  `Rendering/Renderer.h`.
* Remove the 6 `static_cast<RenderingFactory_Vulkan*>` sites in favour of
  `Renderer_Vulkan::Get()`.

Exit criteria: both framework builds succeed; no references to `RenderingFactory` remain.

### Phase 1.5 — Repair the Vulkan compute shader

Pre-existing breakage that Phase 1 exposed once the Vulkan target could finally reach the backend:

* `ComputeShader_Vulkan.h` declared `LoadShaderModule(const std::string&, ...)` while the
  implementation and both call sites pass a `std::vector<char>` SPIR-V blob. Align the declaration.
* `SpvReflectDescriptorSet::bindings` is a `SpvReflectDescriptorBinding**`, not a
  `VkDescriptorSetLayoutBinding*`. Convert each reflected binding (binding index, descriptor type,
  array dimension count, shader stage) into a `std::vector<VkDescriptorSetLayoutBinding>` and pass
  that to `vkCreateDescriptorSetLayout`.
* `Run()` called `Renderer_Vulkan::BeginSingleTimeCommands()` / `EndSingleTimeCommands()`, which
  never existed. Record into a transient graphics command buffer
  (`AllocateTransientCommandBuffer(CommandQueueType::Graphics)`), submit it through the graphics
  queue with `bWait = true`, and release it.

Scope: compile-correctness only. The `SetUniform*` / `SetInput/OutputUniformImage` placeholders in
the Vulkan compute shader are intentionally left unimplemented; descriptor binding management is
part of Phase 7.

Exit criteria: `Vulkan-Debug` builds `GraphicsFramework` and links `UnitTests`.

### Phase 2 — Backend seam

* Add `RHI/Backend.h` declaring `rhi::CreateDevice()` (returns `Renderer*`) and
  `rhi::CreateWindow()` (returns `Window*`).
* Add `Rendering/OpenGL/Backend_OpenGL.cpp` and `Rendering/Vulkan/Backend_Vulkan.cpp`, each defining
  both symbols. They stay inside the existing backend folders so the current CMake filter compiles
  exactly one of them; Phase 4 relocates them to `RHI/OpenGL/BackendGL.cpp` /
  `RHI/Vulkan/BackendVK.cpp`.
* `Renderer::Instance()` becomes `static Renderer* instance = rhi::CreateDevice();`.
* `Window` no longer inherits `SingletonByFactory`; `Window::Instance()` is implemented explicitly
  via `rhi::CreateWindow()`. Delete `WindowFactory` and the now-unused `SingletonByFactory`
  template.
* Remove the unused backend-conditional `<SDL_vulkan.h>` include from `Engine.cpp`.
* Ownership stays raw-pointer, process-lifetime singletons (matching the Phase 1 decision); the
  `std::unique_ptr<Device>` switch lands with `Device` in Phase 4.

Exit criteria: no `#if defined(RENDERER_*)` remains in `Renderer.cpp` or `Window.cpp`, and the
backend symbols come only from the translation unit selected by the build.

### Phase 3 — CMake backend isolation

Goal: **only the selected backend's sources are compiled and linked**; a configuration can never
accidentally contain both backends.

Implemented in `Source/CMakeLists.txt` with three pieces:

1. **Config-scoped source selection.** `GF_OPENGL_SOURCES` / `GF_VULKAN_SOURCES` are globbed
   separately and added to `GraphicsFramework` with one plain per-config
   `target_sources` entry per file:
   ```cmake
   foreach(source IN LISTS GF_VULKAN_SOURCES)
       target_sources(GraphicsFramework PRIVATE
           "$<$<OR:$<CONFIG:Debug_Vulkan>,$<CONFIG:Release_Vulkan>>:${source}>")
   endforeach()
   ```
   Unlike `$<TARGET_OBJECTS:...>`, a plain source entry is filtered per configuration before the
   build graph is emitted, so the other backend has no build rule at all.
2. **Per-backend targets for build requirements.** `GF_RHI_OpenGL` and `GF_RHI_Vulkan` are
   `INTERFACE` targets that carry the API-specific compile definitions (`RENDERER_OPENGL`,
   `GLEW_STATIC`, `IMGUI_IMPL_OPENGL_LOADER_GLEW` / `RENDERER_VULKAN`,
   `SPIRV_REFLECT_USE_SYSTEM_SPIRV_H`, `VK_ENABLE_BETA_EXTENSIONS`) and dependencies
   (`GF_RHI_Common` / `vulkan_headers`). `GraphicsFramework` links the matching one through a
   `$<CONFIG:...>` expression. Interface targets have no build steps, so CMake's habit of adding
   order-only dependencies for every target named inside a generator expression is harmless.
   `GF_RHI_Common` holds the include directories and definitions shared by both backends.
3. **Configure-time validation.** `GF_OPENGL_CONFIGS` / `GF_VULKAN_CONFIGS` map configuration names
   to backends; a loop fails configuration with a readable message when a configuration maps to
   neither or to both:
   ```
   CMake Error at GraphicsFramework/Source/CMakeLists.txt:41 (message):
     Configuration 'Debug_Metal' selects no RHI backend.  Add it to
     GF_OPENGL_CONFIGS or GF_VULKAN_CONFIGS in Source/CMakeLists.txt.
   ```

**Why not `add_library(GF_RHI_OpenGL STATIC ...)`?** With Ninja Multi-Config, CMake resolves target
dependencies referenced inside a generator expression for *every* configuration. A
`$<$<CONFIG:Debug_OpenGL>:GF_RHI_OpenGL>` link plus its `GraphicsFramework` back-edge (backend code
calls `Engine`/`ObjectManager`/`ShaderRegistry`, the framework calls `rhi::CreateDevice`) formed a
dependency cycle that pulled **both** backends into both configurations. `EXCLUDE_FROM_ALL`, static
libraries, and object libraries linked via `$<TARGET_OBJECTS:...>` all exhibited the same problem —
the object variant additionally failed to build the Vulkan backend in the OpenGL configuration
because it inherited neither `vulkan_headers` nor the Vulkan definitions. Config-scoped source
selection avoids the cross-target dependency entirely.

Also in this phase:

* `ThirdParty/CMakeLists.txt`: `imgui` links `vulkan_headers` only for Vulkan configurations, so
  OpenGL builds no longer depend on Vulkan headers.
* Vulkan-only include directories (`vulkan/include`, `Vulkan_INCLUDE_DIRS`, `SPIRV-Headers`,
  `spirv_reflect`, `volk`, `vma`) moved from `GF_RHI_Common` to `GF_RHI_Vulkan`; the unconditional
  `target_link_libraries(GraphicsFramework PUBLIC vulkan_headers)` is gone. OpenGL configurations no
  longer have Vulkan headers on their include path at all.
* `CONFIGURE_DEPENDS` is kept, so adding backend files does not require a manual reconfigure.

Exit criteria (verified): `OpenGL-*` and `Vulkan-*` configurations each build `GraphicsFramework`
and link `UnitTests`; the generated `CMakeFiles/impl-<Config>.ninja` contains build rules for
exactly one backend (12 OpenGL / 0 Vulkan for OpenGL, 0 / 22 for Vulkan in all four
configurations); misconfiguring the backend mapping fails at configure time.

### Phase 4 — Folder restructure and rename

* Move `Rendering/OpenGL/*` → `RHI/OpenGL/*`, `Rendering/Vulkan/*` → `RHI/Vulkan/*`.
* Move `Mesh`, `Model` → `Assets/`; `Quaternion` → `Utils/Math/`.
* Rename backend files and classes to namespace + `GL`/`VK` suffix: `Renderer_OpenGL.{h,cpp}` →
  `DeviceGL.{h,cpp}` (`rhi::gl::Device`), `Renderer_Vulkan.*` → `DeviceVK.*` (`rhi::vk::Device`),
  `Window_OpenGL` → `rhi::gl::WindowGL`, `Texture_Vulkan` → `rhi::vk::TextureVK`,
  `VulkanHelper` → `rhi::vk::Helper`, `CommandBuffer_Vulkan` → `rhi::vk::CommandBufferVK`, etc.
* Wrap every backend implementation in `namespace rhi::gl` / `namespace rhi::vk`, including its
  forward declarations; the `rhi::CreateDevice` / `rhi::CreateWindow` definitions in
  `BackendGL.cpp` / `BackendVK.cpp` stay at `rhi` scope.
* Update `Source/CMakeLists.txt` filters to the new paths (the globs from Phase 3 are the only
  place backend paths appear).
* Update includes in framework and samples.

Exit criteria (verified): the abstract interfaces under `Rendering/` and the framework code under
`Core/`, `Assets/` and `Utils/` include no backend header, with the single documented exception of
`Core/ImguiManager.cpp`, which keeps its backend includes behind `#if defined(RENDERER_*)` until the
Phase 6 capability seam removes them. `GraphicsFramework` and `UnitTests` build in all four
configurations.

Notes from the implementation:

* `Renderer_OpenGL::Device` / `Renderer_Vulkan::Device` data members became `mDevice` (and
  `GetDevice()` returns `mDevice`). A member named `Device` inside `class Device` hides the injected
  class name, which breaks `static Device* Get()`; the `m` prefix removes the ambiguity.
* Keeping the `GL`/`VK` class suffix is a deliberate deviation from "drop the backend suffix",
  see the naming note in section 4.

### Phase 5 — Vulkan device helpers and VMA

* `CreateBuffer` / `CreateImage` / `FindMemoryType` become `DeviceVK` members (or move into a
  `vk::Allocator`); `BufferInfo` / `ImageInfo` become `VkBufferHandle` / `VkImageHandle` in
  `RHI/Vulkan/VulkanTypes.h`.
* Replace hand-rolled `vkGet*MemoryRequirements` + `vkAllocateMemory` + `vkBind*Memory` with
  `vmaCreateBuffer` / `vmaCreateImage` (`VulkanMemoryAllocator` is already linked but unused).
* Fix API details while there: `FindMemoryType` must not return `-1` as `uint32_t`.

Current state after Phase 4: `CreateBuffer`, `CreateImage` and `FindMemoryType` are already members
of `rhi::vk::Device` (they moved there when the factory was deleted), and `FindMemoryType` now
returns `0` after printing a diagnostic instead of wrapping `-1`. The remaining work is the VMA
swap: two `vkAllocateMemory` call sites in `RHI/Vulkan/DeviceVK.cpp`.

Exit criteria: no `vkAllocateMemory` call remains in framework code; Vulkan build and swapchain
still render.

### Phase 6 — Backend capability seam

* `ImguiManager` calls a small backend hook (`InitImGui` / `NewFrame` / `Render`) instead of five
  `#if` blocks and `static_cast<rhi::vk::Device*>` / `static_cast<rhi::gl::WindowGL*>`.
* `Renderer::GetClipSpace()` (or `Device::GetProjectionYFlip()`) replaces
  `Camera.cpp`'s `#if defined(RENDERER_VULKAN)`.

Exit criteria: no `#if defined(RENDERER_*)` in `Core/`.

### Phase 7 — Pipeline objects (optional)

* Introduce `Device::CreatePipeline(shader, vertexLayout, targetFormats, state)`.
* `BeginRenderPass(framebuffer)` / `EndRenderPass()` become the only way to switch targets.
* Remove `Shader::Init()`'s implicit render-target coupling and the GL-only
  `Renderer::BindFrameBuffer` no-op semantics.

Reason for last: this changes the public draw API and every sample, so it should follow the
structural work.

---

## 6. Conventions during the migration

* Keep one logical change per phase; do not mix the rename with behaviour changes.
* Never add a new `#if defined(RENDERER_*)` outside CMake and the backend seam.
* New files under `Source/` are picked up by `CONFIGURE_DEPENDS` globs; backend files must live
  under the backend folder so the Phase 3 targets stay correct.
* Samples that are already broken stay untouched except for mechanical renames.

---

## 7. Progress tracking

| Phase | State | Notes |
|---|---|---|
| 0 | ✅ done | `RHI/Types.h`; `Renderer.h` no longer pulls GLEW/SDL; destructor + `const char*` API name |
| 1 | ✅ done | factory/selector deleted; `Create*` on `Renderer`; `VulkanTypes.h` added; 98 call sites rewritten |
| 1.5 | ✅ done | `LoadShaderModule` signature aligned; spv-reflect bindings converted; `Run()` uses a transient graphics command buffer |
| 2 | ✅ done | `RHI/Backend.h` + per-backend `CreateDevice`/`CreateWindow`; `Renderer.cpp` / `Window.cpp` have no `#if`; `SingletonByFactory` deleted |
| 3 | ✅ done | config-scoped backend sources + `GF_RHI_OpenGL`/`GF_RHI_Vulkan` interface targets + configure-time single-backend check; `imgui` → `vulkan_headers` scoped to Vulkan |
| 4 | ✅ done | `RHI/{OpenGL,Vulkan}` + `Assets/` + `Utils/Math/`; classes namespaced (`rhi::gl`, `rhi::vk`) and renamed to `DeviceGL`/`DeviceVK`/`TextureGL`/`TextureVK`/…; `VkDevice Device` → `mDevice` |
| 5 | ⬜ not started | |
| 6 | ⬜ not started | |
| 7 | ⬜ optional | |

### Verification status after Phase 3

* `OpenGL-Debug`, `OpenGL-Release`, `Vulkan-Debug`, `Vulkan-Release` → `GraphicsFramework` and
  `UnitTests` build and link.
* Build rules per configuration (`CMakeFiles/impl-<Config>.ninja`):

  | Configuration | OpenGL sources | Vulkan sources |
  |---|---|---|
  | `Debug_OpenGL` | 12 | 0 |
  | `Debug_Vulkan` | 0 | 22 |
  | `Release_OpenGL` | 12 | 0 |
  | `Release_Vulkan` | 0 | 22 |

* A configuration added to `CMAKE_CONFIGURATION_TYPES` without a backend mapping fails at
  configure time with an explicit message.
* `Renderer::Instance()` and `Window::Instance()` resolve through `rhi::CreateDevice()` /
  `rhi::CreateWindow()`; no backend switch remains in either file.
* `UnitTests` runs remain the end-to-end check for both backends (see section 9).

### Verification status after Phase 4

* All four configurations build `GraphicsFramework` and link `UnitTests`
  (`OpenGL-Debug`, `OpenGL-Release`, `Vulkan-Debug`, `Vulkan-Release`).
* Build rules per configuration still contain exactly one backend: 11 OpenGL / 0 Vulkan in the two
  OpenGL configurations and 0 / 21 in the two Vulkan configurations (one source fewer than the
  Phase 3 table because the deleted `RenderingFactory_*` translation units no longer exist, not
  because a backend file moved out of the per-config source set).
* Layout: `Source/RHI/OpenGL` (21 files), `Source/RHI/Vulkan` (22 files + 20 in `Internal/`),
  `Source/Assets/Mesh.*` + `Model.*`, `Source/Utils/Math/Quaternion.*`.
* `grep -r "\b\w+_OpenGL\b\|\b\w+_Vulkan\b"` over the framework, samples and `UnitTests` returns
  only SDL / ImGui backend names (`SDL_WINDOW_OPENGL`, `imgui_impl_vulkan.h`) — no leftover
  `Renderer_Vulkan`, `Texture_OpenGL`, … identifiers.
* No framework file outside `Source/RHI/` includes a backend header except `Core/ImguiManager.cpp`
  (Phase 6 scope). `Core/Camera.cpp` still has one `#if defined(RENDERER_VULKAN)` (also Phase 6).
* `rhi::CreateDevice()` / `rhi::CreateWindow()` are defined once per configuration in
  `RHI/OpenGL/BackendGL.cpp` / `RHI/Vulkan/BackendVK.cpp` and allocate the namespaced classes.

Build note: `UnitTests` must be linked from an **x64** MSVC environment
(`vcvars64.bat`). In an x86 shell the static library still builds, but executables fail to link
with unresolved `operator new`.

---

## 8. Acceptance criteria

A phase is complete when:

1. `cmake --build --preset OpenGL-Debug --target GraphicsFramework --parallel` succeeds.
2. `cmake --build --preset Vulkan-Debug --target GraphicsFramework --parallel` succeeds
   (or the phase documents why it cannot be verified yet).
3. `UnitTests` still links for the configurations that linked before the phase.
4. `git grep -n "RenderingFactory\|RenderingFactorySelector"` returns no hits (from Phase 1 on).
5. No new `#if defined(RENDERER_*)` outside CMake and the backend seam.

---

## 9. End-to-end verification with UnitTests

Once the structural phases are complete, the acceptance check is behavioural, not just a successful
compile: **the same `UnitTests` binary behaviour must be observed on both backends.**

Build both configurations:

```powershell
cmake --build --preset OpenGL-Debug --target UnitTests --parallel
cmake --build --preset Vulkan-Debug --target UnitTests --parallel
```

Run each binary with its working directory set to the output folder (the build copies `res/` next
to the executable):

```powershell
build\ninja-multi\GraphicsFramework_Binary\Debug_OpenGL\UnitTests.exe
build\ninja-multi\GraphicsFramework_Binary\Debug_Vulkan\UnitTests.exe
```

For each backend, select every scene from the ImGui *Select Unit Test* combo and confirm:

| Scene | Expected |
|---|---|
| `ShaderTest` | single lit triangle/quad, no shader compile or SPIR-V load errors |
| `VertexBufferTest` | geometry interpolates/animates; vertex layout matches |
| `IndexBufferTest` | indexed quad renders with correct winding, no missing faces |
| `UniformBufferTest` | per-object transform updates; no descriptor/binding validation errors |
| `TextureTest` | JPEG texture sampled correctly; no upside-down or blank image |

A backend passes when all five scenes render and the console shows no Vulkan validation errors
(debug build) or OpenGL `GLCall` assertions.

### Known prerequisite: UnitTests SPIR-V shaders

The Vulkan runtime loads `res/spirv/<name>.spv`, while `UnitTests/CMakeLists.txt` currently emits
SPIR-V into `UnitTests/spirv/` (and the utility target is not wired into the generated build). Before
running the Vulkan verification this must be corrected: emit to `res/spirv/` (and `res/spirv/Vulkan/`
for the Vulkan shader variants) so the post-build `res/` copy carries them next to the executable.
This is a build-packaging fix, not an RHI change, and is tracked as part of the final verification
step.
