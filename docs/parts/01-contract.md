# Sample App RHI Refactor Plan — OpenGL + Vulkan

**Audience:** an execution model with a small context window (target: ~27B parameters, 4-bit quantised).
**Goal:** port the four remaining sample applications to the framework's RHI so that each one builds and renders on **both** `OpenGL-*` and `Vulkan-*` presets, using the same patterns as `FlockingParticles` and `UnitTests`.

**Targets (in execution order):**

| # | Project | Track | Difficulty | Why this order |
|---|---|---|---|---|
| 1 | `WaveSimulation` | A — portable today | Low | Smallest surface: one scene, two shaders, one grid. Proves the conventions. |
| 2 | `AnimationTechniques` | A — portable today | Medium | Two scenes, skinned meshes, per-draw UBO pool, line drawing. |
| 3 | `RealTimeRayTracing` | A — portable today | Medium | Rewritten as a fragment-shader ray tracer (compute + SSBO path is not portable). |
| 4 | `AdvancedRenderingTechniques` | B — needs framework work | Very high | MRT, offscreen targets, per-pass pipelines, HDR upload, compute descriptors. |

Track A targets must be finished and committed before Track B starts. Track B is split into a **shader-clean milestone** (both presets build, OpenGL runs) and four **framework milestones** (P5–P8) that add the Vulkan capabilities ART needs.

---

## 0. Executor contract

Read this section before touching a file. It is not optional.

### 0.1 How to work

1. Work **one task card at a time**, in the order given by section 11 (dependency graph).
2. A task card is finished only when **every** command in its *Verify* block has been run and produced the stated result. "It looks right" is not a result.
3. After each task card, run the **project gate** (section 12) for that project and preset. If the gate fails, fix it before moving on. Do not start the next card with a red gate.
4. Commit after every green task card using the message in the card (`Commit:` line). One card = one commit.
5. When a card says *replace the whole file*, use the `write` tool with exactly the given content. When it says *edit*, use the `edit` tool with the given anchor text.
6. If a card's instructions do not match the file you are looking at (different content, missing symbol, different line numbers), **stop** and report a mismatch. Do not improvise.

### 0.2 Hard rules

| Rule | Reason |
|---|---|
| **Never** add `#include <GL/...>`, `<GLEW/...>`, `gl*` calls, `glm::` GL headers, or `GL_*` constants to sample code. | Samples must compile with `RENDERER_VULKAN`. |
| **Never** call `Shader::SetUniform*` or `ComputeShader::SetUniform*`. They do not exist in the RHI. | Compile error. All data goes through `UniformBuffer`. |
| **Never** use `VertexArray`. It does not exist. | Compile error. Layout lives on `VertexBuffer::SetDataLayout`. |
| **Never** use `Renderer::DrawAllObjects`, `Engine::Instance().pCamera`, `Engine::Instance().scrWidth`, `Camera::mPrevView` on RTT, or `Shader(path,path)` constructors. | Removed APIs. |
| **Never** put two vertex streams in one draw. One `VertexBuffer`, interleaved. | Vulkan binds exactly one vertex buffer (binding 0). |
| **Never** use `IndexType::UInt32` or `UInt8`. | OpenGL `Device::Draw` hardcodes `GL_UNSIGNED_SHORT`. |
| **Never** rely on `glEnable(GL_DEPTH_TEST)`; use `Renderer::Instance()->SetDepthTest(bool)`. | GL-only call. |
| **Never** invent a shader binding number. Use the tables in section 2.5 and appendix B. | Duplicate bindings fail `glslc` and corrupt Vulkan descriptors. |
| **Never** edit framework code unless a task card explicitly lists it under *Files*. | Out-of-scope changes break other samples. |
| **Never** delete a `.spv` by hand and assume it is regenerated at run time. It is generated at **build** time. | `add_framework_spirv` is a build step. |
| **Never** assume `CommandLine::GetValue` returns false when a flag is absent. It returns `true` (bug, see 2.7). | Wrong scene selection. |

### 0.3 Stop conditions

Stop immediately and report if any of these happen:

1. A build fails with an error **not** listed in the card's *Expected errors* section.
2. `glslc` reports an error you cannot map to a rule in appendix A.
3. A Vulkan run produces `VUID-` lines that name a resource created by a card you already completed.
4. The app runs but `verify-scene.ps1` returns exit code `2` after the card's *Verify* block says it must return `0`.
5. A file you must edit contains code that is not described by this plan (someone else changed it).
6. A task card requires a framework capability that P5–P8 has not delivered yet (Track B only).

### 0.4 What "done" means per card

A card is green when **all** of the following hold:

- `cmake --build --preset <preset> --target <target> --parallel` exits `0` for the preset named in the card.
- `scripts\verify-scene.ps1` exits `0` with `content >= 0.5%` for the scene named in the card.
- The stdout/stderr files contain **no** `Vulkan ERROR`, `VUID-`, `Shader compilation failed`, or `Failed to load` lines.
- `git status --porcelain` shows only the files listed in the card.

---

## 1. Definition of done (whole plan)

The plan is complete when this matrix is fully green.

| Target | OpenGL-Debug | OpenGL-Release | Vulkan-Debug | Vulkan-Release | Scene coverage |
|---|---|---|---|---|---|
| `WaveSimulation` | renders grid | renders grid | renders grid | renders grid | single scene |
| `AnimationTechniques` | renders archer | renders archer | renders archer | renders archer | `-scene=1` (Animation), `-scene=2` (Mechanics) |
| `RealTimeRayTracing` | renders traced image | renders traced image | renders traced image | renders traced image | single scene |
| `AdvancedRenderingTechniques` | renders all projects | renders all projects | renders all projects | renders all projects | `-scene=1` (AllProjects), `-scene=2` (MotionBlur) |
| `FlockingParticles` (regression) | unchanged | unchanged | unchanged | unchanged | must not regress |
| `UnitTests` (regression) | 5/5 scenes | 5/5 scenes | 5/5 scenes | 5/5 scenes | scenes 0–4 |

**Rendering acceptance per target** (what the screenshot must contain):

| Target | Must be visible | Clear colour |
|---|---|---|
| WaveSimulation | A lit grid mesh filling the centre, white background | `(1,1,1)` white |
| AnimationTechniques | The animated archer model at screen centre, green floor plane | `(0.2,0.2,0.2)` grey |
| AnimationTechniques Mechanics | A chain of red cubes hanging between two spheres | `(0.2,0.2,0.2)` grey |
| RealTimeRayTracing | A shaded sphere above a checkerboard-free ground plane, sky gradient | `(0.05,0.07,0.12)` dark blue |
| AdvancedRenderingTechniques | Teapots on a floor with shadows and IBL sky dome | `(0,0,0)` black |

**Performance sanity:** no target may take longer than 3 s to produce its first frame after the window appears (`-SettleMs 3000` must still pass).

---

## 2. Context pack

Everything in this section is **verified fact** about the repository at commit `ad2ae33`. Do not re-derive it; use it.

### 2.1 Build system

```
Repo root:        E:\GraphicsFramework
Framework root:   E:\GraphicsFramework\GraphicsFramework\GraphicsFramework
Samples root:     E:\GraphicsFramework\GraphicsFramework\<Sample>
Binary output:    E:\GraphicsFramework\build\ninja-multi\GraphicsFramework_Binary\<Debug|Release>_<OpenGL|Vulkan>\
```

Preset → output folder mapping:

| Preset | CMake config | Folder |
|---|---|---|
| `OpenGL-Debug` | `Debug_OpenGL` | `build\ninja-multi\GraphicsFramework_Binary\Debug_OpenGL` |
| `OpenGL-Release` | `Release_OpenGL` | `build\ninja-multi\GraphicsFramework_Binary\Release_OpenGL` |
| `Vulkan-Debug` | `Debug_Vulkan` | `build\ninja-multi\GraphicsFramework_Binary\Debug_Vulkan` |
| `Vulkan-Release` | `Release_Vulkan` | `build\ninja-multi\GraphicsFramework_Binary\Release_Vulkan` |

Build commands (always run from the repo root, in an x64 MSVC shell; `verify-scene.ps1 -Build` sets that up for you):

```powershell
cmake --build --preset OpenGL-Debug --target WaveSimulation --parallel
cmake --build --preset Vulkan-Debug --target WaveSimulation --parallel
```

`add_framework_app(target, srcDir)` (`GraphicsFramework/CMakeLists.txt:154`) does three things:

1. `file(GLOB_RECURSE *.cpp)` — **new source files are picked up automatically**, but CMake must re-run. `CONFIGURE_DEPENDS` handles that on the next build; if a new `.cpp` is not compiled, delete `build/ninja-multi` and re-configure once with `cmake --preset Ninja-Multi`.
2. `add_framework_spirv(target, resDir)` — compiles every `*.vert`, `*.frag`, `*.comp`, `*.geom`, `*.tesc`, `*.tese` **recursively** under `res/` into `<same dir>/spirv/<name>.spv` with `glslc --target-env=vulkan1.2`.
3. Copies `res/` and the DLLs next to the executable after every build.

**Critical consequence:** step 2 runs for *every* configuration, including OpenGL. A shader that `glslc` rejects breaks the **OpenGL** build too. Therefore every project is ported **shader-first**.

`add_framework_spirv` does **not** glob `*.compute`. Files with that extension are silently ignored, which is why ART's compute shaders have never been compiled. They must be renamed to `.comp`.

### 2.2 Shader pipeline

| Item | Value |
|---|---|
| Compiler | `glslc` from the Vulkan SDK, found via `find_program` (on this machine: `C:\VulkanSDK\1.4.328.1\Bin\glslc.exe`) |
| Target env | `--target-env=vulkan1.2` |
| Output | `<shader dir>/spirv/<file name>.spv` |
| Runtime (GL) | `ShaderRegistry::GetGLSLPath(id, stage)` → source path, compiled at runtime by the driver |
| Runtime (VK) | `ShaderRegistry::GetSPIRVPath(id, stage)` → `.spv` path, loaded by `ShaderVK` |

`ShaderRegistry::RegisterShader(id, glslVert, glslFrag, spirvVert, spirvFrag)` is called once per shader inside `ShaderRegistry::Instance()->Init(basePath, lambda)`. For a shader at `res/shaders/Drawing.vert`, register:

```cpp
reg->RegisterShader(static_cast<int32_t>(MyShaderID::DRAWING),
                    "Drawing.vert", "Drawing.frag",
                    "spirv/Drawing.vert.spv", "spirv/Drawing.frag.spv");
```

The base path is `"res/shaders/"` (or `"res/Shaders/"` — case matters on the copy, and the folder really is capitalised in `AnimationTechniques`).

### 2.3 Portable RHI subset

These are the only calls a sample may use. Signatures are exact.

```cpp
// Creation
Renderer*   R  = Renderer::Instance();
Shader*          s  = R->CreateShader();              s->Init(int32_t id);
VertexBuffer*    vb = R->CreateVertexBuffer();
IndexBuffer*     ib = R->CreateIndexBuffer();
UniformBuffer*   ub = R->CreateUniformBuffer();
Texture*         tx = R->CreateTexture();
FrameBuffer*     fb = R->CreateFrameBuffer();
ComputeShader*   cs = R->CreateComputeShader();

// Vertex / index data
vb->SetDataLayout({ VertexFormat::Vec3, VertexFormat::Vec3 });   // stride computed from formats
vb->AddData(ptr, byteCount);                                     // static, allocates
vb->AddDynamicData(nullptr, byteCount);                          // allocates, no upload
vb->AddSubData(ptr, byteCount, byteOffset);                      // updates in place

ib->AddData(ptr, indexCount, IndexType::UInt16);
unsigned int n = ib->GetCount();

// Uniform data
ub->Init(byteSize, bindingPoint);                 // once
ub->AddData(byteSize, ptr, byteOffset = 0);       // every frame

// Bind
R->BindUniformBuffer(ub, bindingPoint);
R->BindTexture(tx, slot);
R->BindShader(s);                                  // only needed before DrawQuad

// Draw
R->Draw(vb, ib, s);                                // indexed triangles
R->DebugDrawLines(vb, ib, s);                      // indexed GL_LINES / VK_LINE_LIST
R->DebugDraw(vb, ib, s);                           // indexed GL_LINE_LOOP
R->Draw(s, vb, vertexCount, 0);                    // non-indexed triangles
R->DrawQuad(s);                                    // fullscreen quad, locations 0=vec3,1=vec2

// State
R->SetDepthTest(true);
R->SetCullingFace(CullFace::Back | CullFace::Front | CullFace::None);
R->SetClearColor(glm::vec3(r,g,b));
R->Clear();
```

`Renderer::Draw(vb, ib, s)` binds the shader itself on both backends. `DrawQuad` does **not** bind the shader on Vulkan (`Device::Draw` uses `mBoundShader` for descriptor allocation), so always call `R->BindShader(s)` before `DrawQuad`.

### 2.4 Vulkan backend facts that shape the design

These are not opinions; they are the current implementation.

| # | Fact | Consequence for the port |
|---|---|---|
| V1 | Descriptor writes are **deferred to draw time** from `mBoundUniformBuffers[slot]` / `mBoundTextures[slot]` (`DeviceVK.cpp:547-600`). The maps persist across draws. | Every draw that needs different UBO *contents* must use a **distinct `UniformBuffer` object**. Reusing one UBO and re-uploading between draws makes all draws see the last contents. |
| V2 | `ShaderVK::Init` bakes the pipeline against the framebuffer bound **at that moment** (`ShaderVK.cpp:170-243`). | Create one `Shader` object per (shader, render pass) pair. Bind the target framebuffer *before* `Init`. |
| V3 | Only **one** vertex buffer is bound, at binding 0 (`DeviceVK.cpp:505-511`). | Single interleaved VBO. Stride is derived from the attribute formats in location order. |
| V4 | `RenderPassVK::Init` supports **one** colour attachment (+ optional depth). | ART G-buffer needs a new MRT path (P5). |
| V5 | `FrameBufferVK::Init` is broken: it creates a framebuffer from an **uninitialised** `VkImageView` and never creates textures/depth. | Offscreen rendering needs a rewrite (P5). |
| V6 | `DeviceVK::BindFrameBuffer`, `Clear`, `SetBlending`, `UnbindFrameBuffer` are **empty**. | Offscreen passes and blending need work (P5/P6). |
| V7 | Depth test is disabled in the pipeline (`depthTestEnable = VK_FALSE`) and toggled only through `vkCmdSetDepthTestEnable`, but `extendedDynamicState` is **never enabled** (`pEnabledFeatures = nullptr`). | Enable the feature (P0.5) or depth sorting is undefined on Vulkan. |
| V8 | `VertexBufferVK::AddSubData` uploads through a staging buffer that it then **destroys**, and `QueueCopyCommands` ignores the offset. `AddDynamicData(nullptr, n)` memcpy's from `nullptr`. | Must be rewritten to host-visible persistent buffers (P0.5). |
| V9 | `TextureVK::Init(ImageFormat, w, h)` allocates nothing. `Init(file)` for `.hdr` loads then frees the data without uploading. | Offscreen textures and HDR textures need work (P5/P7). |
| V10 | `ComputeShaderVK::SetUniform*` / `SetUniformBlock` are stubs. | Compute passes need UBO bindings instead (P7). |
| V11 | No `VkPhysicalDeviceFeatures` are enabled at all. | Feature-dependent code must be gated (P0.5 enables exactly two features). |
| V12 | `Camera::Init` uses `glm::perspective` without `GLM_FORCE_DEPTH_ZERO_TO_ONE`, so the near half of the depth range is clipped on Vulkan. | Harmless for these scenes (no near-plane geometry). Do not "fix" it; it would change every sample. |

### 2.5 Conventions every ported sample follows

#### Binding allocation

| Binding | Type | Block / name | Layout (std140) | Owner |
|---|---|---|---|---|
| `0` | UBO | `ObjectMatrices` | `mat4 normaltr; mat4 model; mat4 prevModel;` | `Transform` component (`Settings.json: TransformBindingPoint`) |
| `1` | UBO | `MaterialData` | `vec4 diffuse; vec4 specular; float shininess; int lighting;` | `Material` component (`Settings.json: MaterialBindingPoint`) |
| `2` | UBO | `CameraMatrices` | `mat4 projection; mat4 view; mat4 invView;` | scene |
| `3` | UBO | `LightData` | `vec4 position; vec4 color;` | scene |
| `4`+ | UBO | app-specific | see per-project cards | scene |
| `8`+ | sampler | app-specific | see per-project cards | scene |

The first four are **fixed**. `TransformBindingPoint` and `MaterialBindingPoint` are read from `res/Settings.json`; the framework creates those UBOs for every `Transform`/`Material` component, so a scene that uses `ObjectManager` objects gets them for free.

C++ side (must match the GLSL exactly):

```cpp
struct CameraMatricesUBO
{
    glm::mat4 mProjection;   // offset 0
    glm::mat4 mView;         // offset 64
    glm::mat4 mInverseView;  // offset 128
};                           // size 192

struct LightDataUBO
{
    glm::vec4 mPosition;     // offset 0
    glm::vec4 mColor;        // offset 16
};                           // size 32
```

#### Shader conventions

- `#version 450` in every ported shader. (Proven to compile and run on both backends by `FlockingParticles` and `UnitTests`, even though `WindowGL` requests a 4.3 core context.)
- Every vertex input, vertex output and fragment input has an explicit `layout(location = N)`. Fragment outputs too.
- Every uniform that is not a sampler/image lives in a **named** uniform block with `layout(binding = N)`.
- Every sampler/image has `layout(binding = N)`.
- `bool` inside a uniform block is written as `int` on both sides.
- No `#extension` directives, no `compatibility` profile, no `gl_FragColor`, no `gl_FragData`.

#### Geometry conventions

- One interleaved `VertexBuffer` per draw. Attribute locations are the order of `SetDataLayout`'s format list: index 0 → location 0, index 1 → location 1, …
- Indices are `uint16_t`, uploaded with `IndexType::UInt16`. A grid with more than 65535 vertices must be split (none of these samples needs it).
- Culling: `CullFace::None` unless the card says otherwise. `AnimationTechniques` needs `CullFace::Front` for its floor quad, matching the old `glCullFace(GL_FRONT)` behaviour.

#### `res/Settings.json`

Every sample needs one; `Engine::Start(path)` parses it and the window size comes from there:

```json
{
  "WindowSize": [ 1280.0, 720.0 ],
  "TransformBindingPoint": 0,
  "MaterialBindingPoint": 1
}
```

### 2.6 Canonical reference (copy this shape)

`FlockingParticles/src/Main.cpp`:

```cpp
#include <iostream>
#include "Core/Engine.h"
#include "Flocking.h"

int main(int argc, char* args[])
{
    std::cout << "Graphics Framework" << std::endl;
    Engine::Instance()->Start("res/Settings.json");
    Engine::Instance()->SetScene<Flocking>();
    Engine::Instance()->Run();
    Engine::Instance()->Stop();
    return 0;
}
```

Minimal scene skeleton (all four `Scene` methods are pure virtual — a missing `Close()` makes the class abstract and the error is `C2259 'X': cannot instantiate abstract class`):

```cpp
class MyScene : public Scene
{
public:
    ~MyScene() override;
    void Init() override;
    void Close() override;
    void Update() override;
    void RenderObject(Object* object, Shader* shader) override;  // may be empty
    void DebugDisplay() override;                                // may be empty
};
```

Per-frame upload pattern:

```cpp
CameraMatricesUBO cam{ camera->mProjection, camera->mView, glm::inverse(camera->mView) };
mCameraUBO->AddData(sizeof(cam), &cam);
Renderer::Instance()->BindUniformBuffer(mCameraUBO, 2);
```

### 2.7 Framework defects you must work around (do not fix unless a card says so)

| Defect | Where | Work-around used by this plan |
|---|---|---|
| `CommandLine::GetValue` returns `true` when the flag is **absent** (`FoundIdx != CmdStr.size()` instead of `!= npos`). | `Core/CommandLine.cpp:39,54,69` | Samples parse `-scene=` themselves with a local helper (given in each Main.cpp card). Do **not** use `CommandLine::GetValue` for `-scene`. |
| `CommandLine::Init(argc, argv)` is only called by `Core/EntryPoint.h`'s `main`. Samples define their own `main`. | `Core/EntryPoint.h:14` | The local helper reads `argv` directly; no `CommandLine` call at all. |
| `Mesh::Draw` never assigns `name`, so textures bind at slot `i` (0,1,2,…). | `Assets/Mesh.cpp:31,51` | P2 card P2-00 changes it to slot `8 + i`. |
| `Mesh::Draw` binds textures even when the mesh has none; the sampler is then unbound. | `Assets/Mesh.cpp:28-52` | Shaders that sample a mesh texture take an explicit `useTexture` flag (P2). |
| `MaterialDataUBO` has no "has texture" flag. | `Core/Components/Material.h:10` | AnimationTechniques defines its own app-local material block; the framework struct is untouched. |
| `Renderer::DrawQuad` allocates and deletes a VBO/IBO every call. | `Rendering/Renderer.cpp:14` | Acceptable: only used by RTT (once per frame) and ART's fullscreen passes. |

---

## 3. Repository map for this work

| Path | What it is |
|---|---|
| `GraphicsFramework/GraphicsFramework/Source/` | Framework library: `Core/`, `Rendering/`, `RHI/OpenGL/`, `RHI/Vulkan/`, `Assets/`, `Utils/` |
| `GraphicsFramework/GraphicsFramework/Source/RHI/Vulkan/` | Vulkan backend — the files P0.5 and P5–P8 touch |
| `GraphicsFramework/GraphicsFramework/Source/RHI/OpenGL/` | OpenGL backend — only P0.5 touches it (read-only otherwise) |
| `GraphicsFramework/GraphicsFramework/{WaveSimulation,AnimationTechniques,RealTimeRayTracing,AdvancedRenderingTechniques}/` | The four targets: `src/` + `res/` |
| `GraphicsFramework/FlockingParticles/` | Reference implementation — **do not modify** |
| `GraphicsFramework/UnitTests/` | Reference implementation + regression suite — **do not modify** |
| `scripts/verify-scene.ps1` | The only accepted way to prove a frame was rendered |
| `docs/RHI-Migration-Plan.md` | The original migration document; style reference |
| `build/plan-audit/` | Temporary audit build tree created while writing this plan — delete in P9 |

