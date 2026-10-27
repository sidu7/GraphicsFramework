---

## 6. Phase 1 — `WaveSimulation`

**Project root:** `GraphicsFramework/WaveSimulation/`
**Track:** A (portable on the current RHI — no framework work needed beyond P0.5).
**Size:** 2 shaders, 3 source files. This phase establishes the exact pattern the later phases copy, so do it carefully.

### 6.0 What changes and why

| File | Action | Reason |
|---|---|---|
| `res/Settings.json` | **create** | `Engine::Start("res/Settings.json")` needs a settings file; the project has none. |
| `res/shaders/Drawing.vert` | **replace** | `#version 330` + loose uniforms → `#version 450` + UBOs. |
| `res/shaders/Drawing.frag` | **replace** | same. |
| `src/Wave.h` | **replace** | `VertexArray`, two VBOs, `Light*` etc. → RHI handles. |
| `src/Wave.cpp` | **replace** | `glEnable`, `SetUniform*`, `BindVertexArray`, `unsigned` indices → RHI. |
| `src/Main.cpp` | **replace** | `Engine::Start(&proj, 1280, 720)` does not exist. |
| `src/HeightField.*`, `src/Grid.h`, `src/FFT*.h` | **do not touch** | Pure CPU math, no graphics API. |
| `CMakeLists.txt` (inside `WaveSimulation/`) | **do not touch** | Not used by the build (see 2.1). |

Geometry design (this is the part that must be exactly right):

- **One** interleaved `VertexBuffer` with layout `{ Vec3, Vec3 }` = `{ position, normal }`, stride 24.
- **One** `IndexBuffer` of `uint16_t`. The grid is 64×64 = 4096 vertices, so 16-bit indices are enough. `IndexType::UInt16` is mandatory (OpenGL's `Device::Draw` hardcodes `GL_UNSIGNED_SHORT`).
- **Four** `UniformBuffer`s at bindings 0, 1, 2, 3 using the fixed framework layout. `ObjectMatricesUBO` and `MaterialDataUBO` come from the framework headers — include them, do not redefine them.

---

### P1-01 — Add `res/Settings.json`

**Files:** `GraphicsFramework/WaveSimulation/res/Settings.json` (new file)

**Content (exactly):**

```json
{
  "WindowSize": [ 1280.0, 720.0 ],
  "TransformBindingPoint": 0,
  "MaterialBindingPoint": 1
}
```

**Notes:**

- `ShaderBasePath` is parsed by nothing (`Engine::Settings::Parse` reads only the three keys above), so do not add it.
- Every sample writes `res/` into the **same** output folder, so this file will be overwritten by whichever sample was built last. That is fine as long as the contents are equivalent — and they are, byte for byte, with `FlockingParticles/res/Settings.json` apart from the unused `ShaderBasePath` key.

**Verify:**

```powershell
Test-Path GraphicsFramework\WaveSimulation\res\Settings.json
```

Expected: `True`.

**Commit:** `feat(wave): add res/Settings.json so the sample can use Engine::Start`

---

### P1-02 — Rewrite the two shaders

**Files:** `GraphicsFramework/WaveSimulation/res/shaders/Drawing.vert`, `GraphicsFramework/WaveSimulation/res/shaders/Drawing.frag`

#### `Drawing.vert` — replace the whole file

```glsl
#version 450

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

// Per-object matrices, filled by the scene (Settings.json: TransformBindingPoint)
layout (binding = 0) uniform ObjectMatrices
{
	mat4 normaltr;
	mat4 model;
	mat4 prevModel;
} Object;

// Per-frame camera, filled by the scene
layout (binding = 2) uniform CameraMatrices
{
	mat4 projection;
	mat4 view;
	mat4 invView;
} Camera;

layout (location = 0) out vec3 worldPos;
layout (location = 1) out vec3 worldNormal;

void main()
{
	vec4 worldPosition = Object.model * vec4(aPos, 1.0);

	gl_Position = Camera.projection * Camera.view * worldPosition;

	worldPos = worldPosition.xyz;
	worldNormal = normalize((Object.normaltr * vec4(aNormal, 0.0)).xyz);
}
```

#### `Drawing.frag` — replace the whole file

```glsl
#version 450

layout (location = 0) in vec3 worldPos;
layout (location = 1) in vec3 worldNormal;

layout (location = 0) out vec4 FragColor;

// Per-material data (Settings.json: MaterialBindingPoint)
layout (binding = 1) uniform MaterialData
{
	vec4 diffuse;
	vec4 specular;
	float shininess;
	int lighting;
} Material;

// Per-frame light data, filled by the scene
layout (binding = 3) uniform LightData
{
	vec4 position;
	vec4 color;
} Light;

void main()
{
	vec3 L = normalize(Light.position.xyz - worldPos);
	vec3 N = normalize(worldNormal);

	float NL = max(dot(N, L), 0.0);

	FragColor = vec4(NL * Material.diffuse.rgb, 1.0);
}
```

**Why this exact shader and not `FlockingParticles`' fragment shader:** the original wave shader is pure Lambert (`ml * diffuse_color`, no specular). Reusing Flocking's Blinn-Phong shader would add a specular highlight and divide the diffuse term by π, changing the picture. Keep Lambert so the screenshot can be compared to the pre-refactor build.

**Why `Light.position` is a position and not a direction:** the original used `light_position = (1.0, -0.3, -0.1)` as a point light. Do not "correct" it to a direction; the shading would change.

**Verify:**

```powershell
& "C:\VulkanSDK\1.4.328.1\Bin\glslc.exe" --target-env=vulkan1.2 GraphicsFramework\WaveSimulation\res\shaders\Drawing.vert -o $env:TEMP\wave_vert.spv
& "C:\VulkanSDK\1.4.328.1\Bin\glslc.exe" --target-env=vulkan1.2 GraphicsFramework\WaveSimulation\res\shaders\Drawing.frag -o $env:TEMP\wave_frag.spv
```

Expected: both commands print nothing and exit `0`; the two `.spv` files are non-zero bytes.

**Stop if:** `glslc` reports `'non-opaque uniforms outside a block'`. That means a `uniform` declaration was left outside a block — go back and put it in one of the four blocks above.

**Commit:** `feat(wave): port Drawing shaders to GLSL 450 uniform blocks`

---

### P1-03 — Rewrite `Wave.h`

**Files:** `GraphicsFramework/WaveSimulation/src/Wave.h`

**Replace the whole file:**

```cpp
#pragma once
#include <core/Scene.h>
#include <glm/glm.hpp>
#include <vector>
#include "HeightField.h"

class Light;
class Shader;
class VertexBuffer;
class IndexBuffer;
class UniformBuffer;

class Wave : public Scene
{
public:
	Wave() = default;
	~Wave() override;

	void Init() override;
	void Close() override;
	void Update() override;
	void RenderObject(class Object* object, class Shader* shader) override;
	void DebugDisplay() override;

private:
	// One interleaved vertex: position + normal, stride 24 bytes.
	// The attribute order is the order passed to VertexBuffer::SetDataLayout,
	// which is also the GLSL location order (0 = position, 1 = normal).
	struct WaveVertex
	{
		glm::vec3 mPosition;
		glm::vec3 mNormal;
	};

	Light* mLight = nullptr;
	Shader* mShader = nullptr;

	HeightField mHeightMap;

	VertexBuffer* mVertexBuffer = nullptr;
	IndexBuffer* mIndexBuffer = nullptr;

	UniformBuffer* mObjectUBO = nullptr;    // binding 0
	UniformBuffer* mMaterialUBO = nullptr;  // binding 1
	UniformBuffer* mCameraUBO = nullptr;    // binding 2
	UniformBuffer* mLightUBO = nullptr;     // binding 3

	std::vector<WaveVertex> mVertices;

	glm::vec3 mDiffuseColor{ 0.8f, 0.8f, 1.0f };
	float mWaveIntensity = 0.01f;
};
```

**Notes:**

- `Close()` is a new override. `Scene` declares five pure virtuals (`Init`, `Close`, `Update`, `RenderObject`, `DebugDisplay`); the old class was abstract-by-accident and only compiled because it was instantiated on the stack via a non-virtual path. `Engine::SetScene<Wave>()` does `new Wave()`, which requires a concrete class.
- No `VertexArray`, no `mVBOs[2]`, no `mMeshPositions`/`mMeshNormals` (they are merged into `mVertices`), no `mTriCount` (use `mIndexBuffer->GetCount()`).

**Verify:** none on its own (the header is only meaningful with P1-04). Continue.

**Commit:** with P1-04.

---

### P1-04 — Rewrite `Wave.cpp`

**Files:** `GraphicsFramework/WaveSimulation/src/Wave.cpp`

**Replace the whole file:**

```cpp
#include "Wave.h"

#include <cstdint>
#include <iostream>

#include "Core/Camera.h"
#include "Core/Components/Material.h"
#include "Core/Components/Transform.h"
#include "Core/Engine.h"
#include "Core/Inputs.h"
#include "Core/Light.h"
#include "Core/Time.h"
#include "Core/Window.h"
#include "Rendering/IndexBuffer.h"
#include "Rendering/Renderer.h"
#include "Rendering/Shader.h"
#include "Rendering/ShaderRegistry.h"
#include "Rendering/UniformBuffer.h"
#include "Rendering/VertexBuffer.h"

#include "Imgui/imgui.h"

#include <glm/gtc/matrix_transform.hpp>

namespace
{
	// Fixed binding points. 0 and 1 come from res/Settings.json, 2 and 3 are
	// allocated by this scene. See docs section 2.5.
	constexpr uint32_t ObjectBindingPoint = 0;
	constexpr uint32_t MaterialBindingPoint = 1;
	constexpr uint32_t CameraBindingPoint = 2;
	constexpr uint32_t LightBindingPoint = 3;

	// Project-local shader IDs
	enum class WaveShaderID : int32_t
	{
		NONE = 0,
		DRAWING,
	};

	// ObjectMatricesUBO and MaterialDataUBO are the framework's structs
	// (Core/Components/Transform.h, Core/Components/Material.h) — reused so the
	// std140 layout cannot drift away from the GLSL blocks.
	struct CameraMatricesUBO
	{
		glm::mat4 mProjection;
		glm::mat4 mView;
		glm::mat4 mInverseView;
	};

	struct LightDataUBO
	{
		glm::vec4 mPosition;
		glm::vec4 mColor;
	};
}

Wave::~Wave()
{
	Close();
}

void Wave::Close()
{
	delete mLight;
	mLight = nullptr;

	delete mShader;
	mShader = nullptr;

	delete mVertexBuffer;
	mVertexBuffer = nullptr;

	delete mIndexBuffer;
	mIndexBuffer = nullptr;

	delete mObjectUBO;
	mObjectUBO = nullptr;

	delete mMaterialUBO;
	mMaterialUBO = nullptr;

	delete mCameraUBO;
	mCameraUBO = nullptr;

	delete mLightUBO;
	mLightUBO = nullptr;
}

void Wave::Init()
{
	std::cout << "Wave Simulation" << std::endl;

	// The registry must be initialised before any Shader::Init call.
	ShaderRegistry::Instance()->Init("res/shaders/", [](ShaderRegistry* reg) {
		reg->RegisterShader(static_cast<int32_t>(WaveShaderID::DRAWING),
							"Drawing.vert", "Drawing.frag",
							"spirv/Drawing.vert.spv", "spirv/Drawing.frag.spv");
	});

	mLight = new Light(glm::vec3(1.0f, -0.3f, -0.1f));

	Camera* camera = Engine::Instance()->GetCamera();
	camera->mCameraPos = glm::vec3(0.1f, 2.0f, 2.2f);
	camera->pitch = -39.0f;
	camera->yaw = -95.0f;
	camera->mSpeed /= 4.0f;
	camera->CalculateFront();

	mShader = Renderer::Instance()->CreateShader();
	mShader->Init(static_cast<int32_t>(WaveShaderID::DRAWING));

	// --- uniform blocks -------------------------------------------------
	mObjectUBO = Renderer::Instance()->CreateUniformBuffer();
	mObjectUBO->Init(sizeof(ObjectMatricesUBO), ObjectBindingPoint);

	mMaterialUBO = Renderer::Instance()->CreateUniformBuffer();
	mMaterialUBO->Init(sizeof(MaterialDataUBO), MaterialBindingPoint);

	mCameraUBO = Renderer::Instance()->CreateUniformBuffer();
	mCameraUBO->Init(sizeof(CameraMatricesUBO), CameraBindingPoint);

	mLightUBO = Renderer::Instance()->CreateUniformBuffer();
	mLightUBO->Init(sizeof(LightDataUBO), LightBindingPoint);

	// Material never changes, so it is uploaded once.
	MaterialDataUBO materialData{};
	materialData.mDiffuse = glm::vec4(mDiffuseColor, 1.0f);
	materialData.mSpecular = glm::vec4(0.0f);
	materialData.mShininess = 1.0f;
	materialData.mLighting = 1;
	mMaterialUBO->AddData(sizeof(materialData), &materialData);

	// --- geometry -------------------------------------------------------
	mHeightMap.initialize();

	const int width = mHeightMap.grid.width();
	const int height = mHeightMap.grid.height();
	const int vertexCount = width * height;

	mVertices.resize(static_cast<size_t>(vertexCount));

	mVertexBuffer = Renderer::Instance()->CreateVertexBuffer();
	mVertexBuffer->SetDataLayout({ VertexFormat::Vec3, VertexFormat::Vec3 });
	// Allocates storage on both backends; contents are written every frame.
	mVertexBuffer->AddDynamicData(nullptr, static_cast<unsigned int>(vertexCount * sizeof(WaveVertex)));

	// Two triangles per grid cell, uint16 indices (DeviceGL::Draw hardcodes
	// GL_UNSIGNED_SHORT, so 32-bit indices are not an option).
	std::vector<uint16_t> indices;
	indices.reserve(static_cast<size_t>(6) * (width - 1) * (height - 1));

	for (int j = 1; j < height; ++j)
	{
		for (int i = 1; i < width; ++i)
		{
			const uint16_t v00 = static_cast<uint16_t>(width * (j - 1) + (i - 1));
			const uint16_t v10 = static_cast<uint16_t>(width * (j - 1) + i);
			const uint16_t v11 = static_cast<uint16_t>(width * j + i);
			const uint16_t v01 = static_cast<uint16_t>(width * j + (i - 1));

			indices.push_back(v00);
			indices.push_back(v10);
			indices.push_back(v11);

			indices.push_back(v00);
			indices.push_back(v11);
			indices.push_back(v01);
		}
	}

	mIndexBuffer = Renderer::Instance()->CreateIndexBuffer();
	mIndexBuffer->AddData(indices.data(), static_cast<unsigned int>(indices.size()), IndexType::UInt16);

	// --- state ----------------------------------------------------------
	Renderer::Instance()->SetClearColor(glm::vec3(1.0f, 1.0f, 1.0f));
	Renderer::Instance()->SetDepthTest(true);
	// The grid is a single-sided sheet; culling it would make the picture depend
	// on the winding order, which differs between the two backends' conventions.
	Renderer::Instance()->SetCullingFace(CullFace::None);

	mWaveIntensity = 0.01f;
}

void Wave::Update()
{
	Camera* camera = Engine::Instance()->GetCamera();

	CameraMatricesUBO cameraData{ camera->mProjection, camera->mView, glm::inverse(camera->mView) };
	mCameraUBO->AddData(sizeof(cameraData), &cameraData);

	LightDataUBO lightData{ glm::vec4(mLight->position, 1.0f), glm::vec4(1.0f) };
	mLightUBO->AddData(sizeof(lightData), &lightData);

	// The grid is built in (i, j, height) space and mapped onto the XZ plane.
	const glm::mat4 I = glm::mat4(1.0f);
	const glm::mat4 modelTrans =
		glm::rotate(I, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f)) *
		glm::translate(I, glm::vec3(-1.0f, -1.0f, 0.0f)) *
		glm::scale(I, glm::vec3(2.0f / mHeightMap.grid.width(),
								2.0f / mHeightMap.grid.height(),
								1.0f));

	// normaltr must be the inverse of the model matrix (the old shader used
	// exactly this), not the transposed inverse, to keep the shading identical.
	ObjectMatricesUBO objectData{ glm::inverse(modelTrans), modelTrans, modelTrans };
	mObjectUBO->AddData(sizeof(objectData), &objectData);

	Renderer::Instance()->BindUniformBuffer(mObjectUBO, ObjectBindingPoint);
	Renderer::Instance()->BindUniformBuffer(mMaterialUBO, MaterialBindingPoint);
	Renderer::Instance()->BindUniformBuffer(mCameraUBO, CameraBindingPoint);
	Renderer::Instance()->BindUniformBuffer(mLightUBO, LightBindingPoint);

	// Rebuild positions and normals on the CPU, then upload the whole
	// interleaved buffer in one call.
	const int width = mHeightMap.grid.width();
	const int height = mHeightMap.grid.height();

	for (int j = 0; j < height; ++j)
	{
		for (int i = 0; i < width; ++i)
		{
			const int index = width * j + i;

			const float h = glm::clamp(mHeightMap.grid(i, j).real(), 0.0f,
									   mHeightMap.max_height * mWaveIntensity);

			mVertices[index].mPosition = glm::vec3(static_cast<float>(i),
												   static_cast<float>(j), h);

			float dx = 0.0f;
			float dy = 0.0f;

			if (i > 0 && i + 1 < width)
			{
				dx = 0.5f * (mHeightMap.grid(i + 1, j).real() - mHeightMap.grid(i - 1, j).real());
			}
			else if (i > 0)
			{
				dx = mHeightMap.grid(i, j).real() - mHeightMap.grid(i - 1, j).real();
			}
			else
			{
				dx = mHeightMap.grid(i + 1, j).real() - mHeightMap.grid(i, j).real();
			}

			if (j > 0 && j + 1 < height)
			{
				dy = 0.5f * (mHeightMap.grid(i, j + 1).real() - mHeightMap.grid(i, j - 1).real());
			}
			else if (j > 0)
			{
				dy = mHeightMap.grid(i, j).real() - mHeightMap.grid(i, j - 1).real();
			}
			else
			{
				dy = mHeightMap.grid(i, j + 1).real() - mHeightMap.grid(i, j).real();
			}

			mVertices[index].mNormal = glm::vec3(-dx, -dy, 1.0f);
		}
	}

	mVertexBuffer->AddSubData(mVertices.data(),
							  static_cast<unsigned int>(mVertices.size() * sizeof(WaveVertex)),
							  0);

	Renderer::Instance()->Draw(mVertexBuffer, mIndexBuffer, mShader);

	if (Inputs::Instance()->IsMouseClicked(SDL_BUTTON_LEFT))
	{
		glm::vec2 mousePos = Inputs::Instance()->GetMouseLocation();
		mousePos /= Window::Instance()->GetWindowSize();
		mHeightMap.handle_input(mousePos);
	}

	mHeightMap.update(Time::Instance()->deltaTime);
}

void Wave::RenderObject(Object* object, Shader* shader)
{
	// This scene draws directly; ObjectManager objects are not used.
}

void Wave::DebugDisplay()
{
	if (ImGui::Button("Reset"))
	{
		mHeightMap.initialize();
	}
	ImGui::SliderFloat("Wave Intensity", &mWaveIntensity, 0.01f, 1.f);
}
```

**Notes:**

- **Do not** call `Renderer::Clear()` in `Update()`. Both backends clear at `StartFrame` (`DeviceGL::StartFrame` → `Clear()`, `DeviceVK::StartFrame` → `BeginScreenPass`, which clears on load). The old `glClearDepth(1)` is gone with it.
- **Do not** add a second vertex buffer for normals. Vulkan binds exactly one vertex buffer at binding 0 and derives the stride from the attribute formats in location order.
- `mVertexBuffer->AddDynamicData(nullptr, …)` is required on OpenGL: `glBufferSubData` in `Update()` needs a store that was created with `glBufferData` first. On Vulkan it just reserves capacity.
- `Wave::Close()` is idempotent (every pointer is set to `nullptr`), so the `~Wave()` → `Close()` path after `Engine::Stop()` → `Close()` is safe.
- The `#include <core/Scene.h>` inside `Wave.h` uses a lowercase `core`; that is the existing spelling and works on this case-insensitive filesystem. Do not "fix" it, other headers may be equally inconsistent.

**Expected errors (and only these):**

| Error | Cause |
|---|---|
| `C2065 'SDL_BUTTON_LEFT': undeclared identifier` | `Core/Inputs.h` was not included, or SDL headers moved. Re-add the include; do not replace the constant. |
| `C2039 'SetUniform3f' is not a member of 'Shader'` | old code left behind — the file was not fully replaced. |
| `C2259 'Wave': cannot instantiate abstract class` | `Close()` is missing or its signature differs from `void Close() override`. |

**Verify:**

```powershell
cmake --build --preset OpenGL-Debug --target WaveSimulation --parallel
```

Expected: exit `0`, and `build\ninja-multi\GraphicsFramework_Binary\Debug_OpenGL\res\shaders\spirv\Drawing.vert.spv` exists with non-zero size.

**Commit:** `feat(wave): rewrite scene against the RHI (single interleaved VBO, UBOs, uint16 indices)`

---

### P1-05 — Rewrite `Main.cpp`

**Files:** `GraphicsFramework/WaveSimulation/src/Main.cpp`

**Replace the whole file:**

```cpp
#include <iostream>

#include "Core/Engine.h"
#include "Wave.h"

int main(int argc, char* args[])
{
	std::cout << "Wave Simulation" << std::endl;

	Engine::Instance()->Start("res/Settings.json");
	Engine::Instance()->SetScene<Wave>();
	Engine::Instance()->Run();
	Engine::Instance()->Stop();

	return 0;
}
```

**Why:** `Engine::Start(&proj, 1280, 720)` no longer exists. The scene is now created by `SetScene<Wave>()` so that `Engine::ClearScene()`/`Engine::Stop()` own its lifetime.

**Verify:**

```powershell
cmake --build --preset OpenGL-Debug --target WaveSimulation --parallel
cmake --build --preset Vulkan-Debug --target WaveSimulation --parallel
```

Expected: both exit `0`.

**Commit:** `feat(wave): start the scene through Engine::SetScene and res/Settings.json`

---

### P1-06 — Gate: `WaveSimulation` renders on both backends

Run all four presets. Do not skip Release — it compiles with different flags and catches unused-variable and shadowing problems that Debug hides.

```powershell
cmake --build --preset OpenGL-Debug   --target WaveSimulation --parallel
cmake --build --preset OpenGL-Release --target WaveSimulation --parallel
cmake --build --preset Vulkan-Debug   --target WaveSimulation --parallel
cmake --build --preset Vulkan-Release --target WaveSimulation --parallel
```

```powershell
powershell -ExecutionPolicy Bypass -File scripts\verify-scene.ps1 -Preset OpenGL-Debug   -BuildTarget WaveSimulation -NoScene -Background "255,255,255"
powershell -ExecutionPolicy Bypass -File scripts\verify-scene.ps1 -Preset OpenGL-Release -BuildTarget WaveSimulation -NoScene -Background "255,255,255"
powershell -ExecutionPolicy Bypass -File scripts\verify-scene.ps1 -Preset Vulkan-Debug   -BuildTarget WaveSimulation -NoScene -Background "255,255,255"
powershell -ExecutionPolicy Bypass -File scripts\verify-scene.ps1 -Preset Vulkan-Release -BuildTarget WaveSimulation -NoScene -Background "255,255,255"
```

**Acceptance (all four runs):**

| Check | Value |
|---|---|
| `RESULT` | `PASS` |
| `VALIDATION` | `no errors in stdout/stderr` |
| `content` | `>= 3%` (the grid covers most of the viewport; a white frame with no grid reports `0%`) |
| `bg` | `(254,254,254)` or `(255,255,255)` with source `manual` |
| `centerMean` | every channel `< 250` (i.e. the centre is shaded geometry, not the clear colour) |
| PNG | a quadrilateral sheet of small quads, visible as a wavy grid, with a light-blue tint |

**If `content` is `0%` and `VALIDATION` is clean:** the draw is being culled or the shader wrote the clear colour. Check, in this order: (1) `SetCullingFace(CullFace::None)` present, (2) `Object.model` uploaded (not a zero matrix), (3) `mMaterialUBO->AddData` called (a zero diffuse gives black, which still counts as content — so this is *not* the cause of `0%`), (4) the index buffer count is `6*(w-1)*(h-1)`.

**If `VALIDATION` reports `VUID-VkGraphicsPipelineCreateInfo-…-depth`:** the depth attachment of the screen pass is missing. That is a framework problem, not a sample problem — report it, do not patch the sample.

**Commit:** `chore(wave): verify WaveSimulation on OpenGL and Vulkan, Debug and Release`

---

### P1-07 — Regression check

The Wave port must not have touched anything shared. Confirm:

```powershell
git status --porcelain
```

Expected: only `GraphicsFramework/WaveSimulation/` paths, plus the framework files from P0.5 (`HelperVK.cpp`, `VertexBufferVK.*`) which are already committed.

```powershell
powershell -ExecutionPolicy Bypass -File scripts\verify-scene.ps1 -Preset Vulkan-Debug -Scene 4
powershell -ExecutionPolicy Bypass -File scripts\verify-scene.ps1 -Preset OpenGL-Debug -BuildTarget FlockingParticles -NoScene
```

Expected: both `RESULT : PASS`.

**Commit:** none.

---

## 7. Phase 2 — `AnimationTechniques`

**Project root:** `GraphicsFramework/AnimationTechniques/`
**Track:** A.
**Size:** 2 scenes (`AnimationScene`, `Mechanics`), 4 shaders, skinned meshes, a polyline debug draw, and the first per-draw UBO pool.

Read section 2.5 again before starting. This phase introduces four patterns that the rest of the plan reuses:

1. **Per-draw UBO pool** — because Vulkan descriptor writes happen at draw time (V2/V1), two draws that need different contents must use two different `UniformBuffer` objects.
2. **Bone matrices in a UBO** — `mat4[128]` at binding 4, padded with identity.
3. **Line drawing through `DebugDrawLines`** with a `UInt16` index buffer of vertex *pairs*.
4. **A local `-scene=` parser** in `main`, because `CommandLine::GetValue` is broken (2.7).

### 7.0 Inventory and the exact problem list

Current files (all must be inspected before editing):

```
GraphicsFramework/AnimationTechniques/
  src/Main.cpp
  src/AnimationScene.h / .cpp
  src/Mechanics.h / .cpp
  src/Animation.h            (skeleton/pose math — CPU only)
  src/SkinnedMesh.h / .cpp   (if present)
  res/Shaders/Drawing.vert / .frag
  res/Shaders/ModelDraw.vert / .frag
  res/Shaders/spirv/*.spv
  res/data/*.json, *.fbx
```

Known defects, each fixed by a card below:

| # | Defect | Card |
|---|---|---|
| A1 | `Main.cpp` calls `Engine::Start(&scene, w, h)` and uses `CommandLine::GetValue` for scene selection | P2-01 |
| A2 | Shaders are `#version 330` with loose uniforms and `Shader::SetUniform*` | P2-02 |
| A3 | `AnimationScene` uses `VertexArray`, `glEnable`, `glCullFace`, and one `Transform` UBO shared by every object | P2-03 |
| A4 | `Mesh::Draw` binds textures at slot `i` starting from 0, colliding with UBO bindings 0–3 on Vulkan | P2-00 |
| A5 | Skinned mesh needs 100+ bone matrices; the framework `ObjectMatrices` block has no slot for them | P2-02/P2-03 |
| A6 | The curve polyline is drawn with `DebugDraw` (line loop) and an `unsigned` index buffer | P2-03 |
| A7 | `Mechanics` rebuilds a rope every frame and shares one transform UBO between nodes | P2-04 |
| A8 | Both scenes are selected by an ImGui dropdown; `verify-scene.ps1` needs `-scene=N` | P2-01 |

### P2-00 — Fix the mesh texture slot (framework, one line)

**Why:** `Mesh::Draw` binds `mTextures[i]` at slot `i`, i.e. slots 0, 1, 2… Those are the same descriptor slots as the UBO bindings 0–3. On Vulkan a sampler written into slot 0 overwrites the `ObjectMatrices` descriptor and the draw reads garbage. On OpenGL it silently works only because UBO binding points and texture units are different namespaces.

**Files:** `GraphicsFramework/GraphicsFramework/Source/Assets/Mesh.cpp`

**Edit** — find:

```cpp
	Renderer::Instance()->BindTexture(mTextures[i], i);
```

Replace with:

```cpp
	// Samplers live above the uniform-block bindings (0-7) so a mesh texture can
	// never collide with ObjectMatrices/MaterialData/Camera/Light on Vulkan.
	Renderer::Instance()->BindTexture(mTextures[i], 8 + i);
```

**Verify:**

```powershell
cmake --build --preset Vulkan-Debug --target UnitTests --parallel
powershell -ExecutionPolicy Bypass -File scripts\verify-scene.ps1 -Preset Vulkan-Debug -Scene 4
powershell -ExecutionPolicy Bypass -File scripts\verify-scene.ps1 -Preset OpenGL-Debug -Scene 4
```

Expected: both `RESULT : PASS`, `content` unchanged from P0-01 (TextureTest binds exactly one texture, so slot 0 → 8 must not change the image).

**Stop if:** `Scene 4` changes appearance. That would mean a shader hardcodes `layout(binding = 0) uniform sampler2D`. Check `UnitTests/res/TextureTest.frag`; it must use `binding = 8`.

**Commit:** `fix(mesh): bind mesh textures at sampler slot 8 to avoid UBO descriptor collisions`

### P2-01 — `Main.cpp` with a local `-scene=` parser

**Files:** `GraphicsFramework/AnimationTechniques/src/Main.cpp`

**Replace the whole file:**

```cpp
#include <cstring>
#include <iostream>
#include <string>

#include "Core/Engine.h"
#include "AnimationScene.h"
#include "Mechanics.h"

namespace
{
	// -scene=1 -> AnimationScene (skinned archer)
	// -scene=2 -> Mechanics    (rope of cubes)
	// default  -> AnimationScene
	//
	// CommandLine::GetValue cannot be used: it returns true when the flag is
	// absent (Core/CommandLine.cpp compares against CmdStr.size() instead of
	// npos), and sample mains never call CommandLine::Init.
	int ParseSceneArgument(int argc, char* args[])
	{
		const std::string Prefix = "-scene=";

		for (int i = 1; i < argc; ++i)
		{
			const std::string Argument = args[i];
			if (Argument.rfind(Prefix, 0) == 0)
			{
				const std::string Value = Argument.substr(Prefix.size());
				if (!Value.empty())
				{
					return std::atoi(Value.c_str());
				}
			}
		}

		return 1;
	}
}

int main(int argc, char* args[])
{
	std::cout << "Animation Techniques" << std::endl;

	const int SceneIndex = ParseSceneArgument(argc, args);

	Engine::Instance()->Start("res/Settings.json");

	if (SceneIndex == 2)
	{
		Engine::Instance()->SetScene<Mechanics>();
	}
	else
	{
		Engine::Instance()->SetScene<AnimationScene>();
	}

	Engine::Instance()->Run();
	Engine::Instance()->Stop();

	return 0;
}
```

**Notes:**

- `std::atoi` needs `<cstdlib>` on some toolchains. If the build complains, change the include line to `#include <cstdlib>` **and** keep `<cstring>` (both are harmless).
- Do **not** keep the old ImGui scene dropdown as the only selector. The dropdown may stay for interactive use, but the startup argument must win.

**Verify (after P2-02…P2-05 compile):**

```powershell
cmake --build --preset OpenGL-Debug --target AnimationTechniques --parallel
```

**Commit:** with P2-05.

### P2-02 — Shader rewrite (4 files)

**Files:**

- `res/Shaders/Drawing.vert`, `res/Shaders/Drawing.frag` (floor plane and any unskinned geometry)
- `res/Shaders/ModelDraw.vert`, `res/Shaders/ModelDraw.frag` (skinned mesh)

**Binding contract for this project:**

| Binding | Type | Block | Contents |
|---|---|---|---|
| 0 | UBO | `ObjectMatrices` | `mat4 normaltr; mat4 model; mat4 prevModel;` |
| 1 | UBO | `MaterialData` | `vec4 diffuse; vec4 specular; float shininess; int lighting; int useTexture; int pad0; int pad1;` |
| 2 | UBO | `CameraMatrices` | `mat4 projection; mat4 view; mat4 invView;` |
| 3 | UBO | `LightData` | `vec4 position; vec4 color;` |
| 4 | UBO | `BoneMatrices` | `mat4 bones[128];` (skinned shader only) |
| 8 | sampler | `meshTexture` | `sampler2D` |

**Critical detail — the material block is app-local.** The framework's `MaterialDataUBO` is `{vec4, vec4, float, int}` (40 bytes) and has no texture flag. AnimationTechniques defines its **own** 64-byte block with `useTexture` plus two padding ints. Both sides must agree exactly:

```cpp
struct AnimationMaterialUBO
{
	glm::vec4 mDiffuse;    // 0
	glm::vec4 mSpecular;   // 16
	float     mShininess;  // 32
	int32_t   mLighting;   // 36
	int32_t   mUseTexture; // 40
	int32_t   mPad0;       // 44
	int32_t   mPad1;       // 48
	int32_t   mPad2;       // 52
};                         // 64 bytes, 16-byte aligned
```

The GLSL block must therefore be:

```glsl
layout (binding = 1) uniform MaterialData
{
	vec4 diffuse;
	vec4 specular;
	float shininess;
	int lighting;
	int useTexture;
	int pad0;
	int pad1;
	int pad2;
} Material;
```

This block is **only** used by AnimationTechniques shaders. `FlockingParticles` and `UnitTests` keep the 40-byte framework block; do not change them.

#### `res/Shaders/Drawing.vert` — replace the whole file

```glsl
#version 450

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

layout (binding = 0) uniform ObjectMatrices
{
	mat4 normaltr;
	mat4 model;
	mat4 prevModel;
} Object;

layout (binding = 2) uniform CameraMatrices
{
	mat4 projection;
	mat4 view;
	mat4 invView;
} Camera;

layout (location = 0) out vec3 normalVec;
layout (location = 1) out vec3 worldPos;
layout (location = 2) out vec2 texCoord;

void main()
{
	worldPos = (Object.model * vec4(aPos, 1.0)).xyz;
	gl_Position = Camera.projection * Camera.view * vec4(worldPos, 1.0);
	normalVec = aNormal * mat3(Object.normaltr);
	texCoord = aTexCoord;
}
```

#### `res/Shaders/Drawing.frag` — replace the whole file

```glsl
#version 450

layout (location = 0) in vec3 normalVec;
layout (location = 1) in vec3 worldPos;
layout (location = 2) in vec2 texCoord;

layout (location = 0) out vec4 FragColor;

layout (binding = 1) uniform MaterialData
{
	vec4 diffuse;
	vec4 specular;
	float shininess;
	int lighting;
	int useTexture;
	int pad0;
	int pad1;
	int pad2;
} Material;

layout (binding = 3) uniform LightData
{
	vec4 position;
	vec4 color;
} Light;

layout (binding = 8) uniform sampler2D meshTexture;

void main()
{
	vec3 albedo = Material.diffuse.rgb;

	if (Material.useTexture != 0)
	{
		albedo *= texture(meshTexture, texCoord).rgb;
	}

	if (Material.lighting == 0)
	{
		FragColor = vec4(albedo, 1.0);
		return;
	}

	vec3 N = normalize(normalVec);
	vec3 L = normalize(Light.position.xyz - worldPos);
	float NL = max(dot(N, L), 0.0);

	FragColor = vec4(Light.color.rgb * NL * albedo, 1.0);
}
```

#### `res/Shaders/ModelDraw.vert` — replace the whole file

```glsl
#version 450

// Mesh::SetupMesh layout: Vec3 position, Vec3 normal, Vec2 uv, iVec4 boneIDs, Vec4 weights
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in ivec4 aBoneIDs;
layout (location = 4) in vec4 aWeights;

layout (binding = 0) uniform ObjectMatrices
{
	mat4 normaltr;
	mat4 model;
	mat4 prevModel;
} Object;

layout (binding = 2) uniform CameraMatrices
{
	mat4 projection;
	mat4 view;
	mat4 invView;
} Camera;

layout (binding = 4) uniform BoneMatrices
{
	mat4 bones[128];
} Skeleton;

layout (location = 0) out vec3 normalVec;
layout (location = 1) out vec3 worldPos;
layout (location = 2) out vec2 texCoord;

void main()
{
	mat4 skin =
		Skeleton.bones[aBoneIDs.x] * aWeights.x +
		Skeleton.bones[aBoneIDs.y] * aWeights.y +
		Skeleton.bones[aBoneIDs.z] * aWeights.z +
		Skeleton.bones[aBoneIDs.w] * aWeights.w;

	vec4 skinnedPosition = skin * vec4(aPos, 1.0);
	vec3 skinnedNormal = mat3(skin) * aNormal;

	worldPos = (Object.model * skinnedPosition).xyz;
	gl_Position = Camera.projection * Camera.view * vec4(worldPos, 1.0);
	normalVec = skinnedNormal * mat3(Object.normaltr);
	texCoord = aTexCoord;
}
```

#### `res/Shaders/ModelDraw.frag` — replace the whole file

```glsl
#version 450

layout (location = 0) in vec3 normalVec;
layout (location = 1) in vec3 worldPos;
layout (location = 2) in vec2 texCoord;

layout (location = 0) out vec4 FragColor;

layout (binding = 1) uniform MaterialData
{
	vec4 diffuse;
	vec4 specular;
	float shininess;
	int lighting;
	int useTexture;
	int pad0;
	int pad1;
	int pad2;
} Material;

layout (binding = 3) uniform LightData
{
	vec4 position;
	vec4 color;
} Light;

layout (binding = 8) uniform sampler2D meshTexture;

void main()
{
	vec3 albedo = Material.diffuse.rgb;

	if (Material.useTexture != 0)
	{
		albedo *= texture(meshTexture, texCoord).rgb;
	}

	if (Material.lighting == 0)
	{
		FragColor = vec4(albedo, 1.0);
		return;
	}

	vec3 N = normalize(normalVec);
	vec3 L = normalize(Light.position.xyz - worldPos);
	float NL = max(dot(N, L), 0.0);

	FragColor = vec4(Light.color.rgb * NL * albedo, 1.0);
}
```

**Why `useTexture` exists:** `Mesh::Draw` binds a texture even when the mesh has none (2.7). With no texture bound, `texture()` on Vulkan reads an undefined descriptor and the validation layer reports `VUID-vkCmdDraw-None-08114`. The flag makes the sampler read conditional, which removes the error for untextured meshes.

**Why 128 bones:** the archer model has fewer than 64 bones. The array is padded with identity matrices so `aBoneIDs` never indexes past the uploaded data. If a future model has more than 128 bones, **stop** and report; do not silently clamp.

**Verify:**

```powershell
& "C:\VulkanSDK\1.4.328.1\Bin\glslc.exe" --target-env=vulkan1.2 GraphicsFramework\AnimationTechniques\res\Shaders\Drawing.vert   -o $env:TEMP\at_dv.spv
& "C:\VulkanSDK\1.4.328.1\Bin\glslc.exe" --target-env=vulkan1.2 GraphicsFramework\AnimationTechniques\res\Shaders\Drawing.frag   -o $env:TEMP\at_df.spv
& "C:\VulkanSDK\1.4.328.1\Bin\glslc.exe" --target-env=vulkan1.2 GraphicsFramework\AnimationTechniques\res\Shaders\ModelDraw.vert -o $env:TEMP\at_mv.spv
& "C:\VulkanSDK\1.4.328.1\Bin\glslc.exe" --target-env=vulkan1.2 GraphicsFramework\AnimationTechniques\res\Shaders\ModelDraw.frag -o $env:TEMP\at_mf.spv
```

Expected: four silent successes, four non-empty `.spv` files.

**Note the folder name:** it is `res/Shaders/` with a capital `S` in this project. `ShaderRegistry::Init("res/Shaders/", …)` must match exactly, because the path is used verbatim at runtime on a case-sensitive filesystem in CI.

**Commit:** with P2-05.

### P2-03 — Rewrite `AnimationScene`

This card is the largest in Phase 2. Read the *whole* existing `AnimationScene.cpp` before editing; the animation maths (skeleton traversal, keyframe interpolation) stays and only the rendering path changes.

**Files:** `GraphicsFramework/AnimationTechniques/src/AnimationScene.h`, `GraphicsFramework/AnimationTechniques/src/AnimationScene.cpp`

#### Design

| Concern | Old | New |
|---|---|---|
| Vertex data | `Mesh` owns `VertexArray` + separate buffers | `Mesh` already uses `VertexBuffer`/`IndexBuffer` (verify in `Assets/Mesh.cpp`); no change |
| Per-object matrices | one `Transform` UBO from `ObjectManager` | **pool** of `ObjectMatricesUBO`, one per draw, reused round-robin per frame |
| Material | framework `MaterialDataUBO` | `AnimationMaterialUBO` (64 bytes) in a pool |
| Bone matrices | `Shader::SetUniformMat4Array` | `BoneMatrices` UBO at binding 4 |
| Floor | `glCullFace(GL_FRONT)` | `SetCullingFace(CullFace::Front)` |
| Curve | `DebugDraw` + `unsigned` indices | `DebugDrawLines` + `uint16_t` pair indices |
| Depth | `glEnable(GL_DEPTH_TEST)` | `SetDepthTest(true)` |

#### The per-draw UBO pool

Vulkan writes descriptor contents at draw time from `mBoundUniformBuffers` (V1). If one UBO object is re-uploaded between two draws, **both** draws read the second upload. Therefore:

```cpp
// One entry per draw call in a frame. Sized generously; grown on demand.
struct DrawUniforms
{
	UniformBuffer* mObjectUBO = nullptr;    // binding 0
	UniformBuffer* mMaterialUBO = nullptr;  // binding 1
};

std::vector<DrawUniforms> mDrawUniforms;
size_t mDrawIndex = 0;

DrawUniforms& NextDrawUniforms()
{
	if (mDrawIndex >= mDrawUniforms.size())
	{
		DrawUniforms entry;
		entry.mObjectUBO = Renderer::Instance()->CreateUniformBuffer();
		entry.mObjectUBO->Init(sizeof(ObjectMatricesUBO), 0);
		entry.mMaterialUBO = Renderer::Instance()->CreateUniformBuffer();
		entry.mMaterialUBO->Init(sizeof(AnimationMaterialUBO), 1);
		mDrawUniforms.push_back(entry);
	}

	return mDrawUniforms[mDrawIndex++];
}
```

Reset `mDrawIndex = 0` at the top of `Update()`. This grows to the number of distinct draws in one frame and never shrinks; after the first frame it stops allocating.

#### `AnimationScene.h` — replace the whole file

```cpp
#pragma once
#include <core/Scene.h>
#include <glm/glm.hpp>
#include <cstdint>
#include <vector>

#include "Animation.h"

class Shader;
class Light;
class UniformBuffer;
class VertexBuffer;
class IndexBuffer;

// App-local material block. 64 bytes so it stays 16-byte aligned; the extra
// ints are padding, not flags. Must match res/Shaders/*.frag exactly.
struct AnimationMaterialUBO
{
	glm::vec4 mDiffuse;
	glm::vec4 mSpecular;
	float mShininess;
	int32_t mLighting;
	int32_t mUseTexture;
	int32_t mPad0;
	int32_t mPad1;
	int32_t mPad2;
};

class AnimationScene : public Scene
{
public:
	AnimationScene() = default;
	~AnimationScene() override;

	void Init() override;
	void Close() override;
	void Update() override;
	void RenderObject(class Object* object, class Shader* shader) override;
	void DebugDisplay() override;

private:
	struct DrawUniforms
	{
		UniformBuffer* mObjectUBO = nullptr;
		UniformBuffer* mMaterialUBO = nullptr;
	};

	DrawUniforms& NextDrawUniforms();
	void UploadBones();
	void BuildCurveBuffers();

	Light* mLight = nullptr;
	Shader* mDrawingShader = nullptr;
	Shader* mModelShader = nullptr;

	UniformBuffer* mCameraUBO = nullptr;
	UniformBuffer* mLightUBO = nullptr;
	UniformBuffer* mBoneUBO = nullptr;

	std::vector<DrawUniforms> mDrawUniforms;
	size_t mDrawIndex = 0;

	VertexBuffer* mCurveVBO = nullptr;
	IndexBuffer* mCurveIBO = nullptr;

	std::vector<glm::mat4> mBoneMatrices;

	// Animation data (kept from the old implementation)
	Animation mAnimation;
	float mAnimationTime = 0.0f;
	bool mPaused = false;

	glm::vec3 mFloorColor{ 0.2f, 0.45f, 0.2f };
	glm::vec3 mArcherColor{ 0.85f, 0.75f, 0.6f };
};
```

> **Mismatch check:** the existing `AnimationScene.h` has different member names (`mSkeleton`, `mModel`, `mCurves`, …). Keep the members that the animation maths needs and rename only the rendering members listed above. If the existing header has members this skeleton does not mention, keep them — this card replaces the *rendering* members, not the animation state.

#### `AnimationScene.cpp` — required changes

Apply these edits to the existing file; do not blind-replace it.

**Edit 1 — includes.** Remove `#include "Rendering/VertexArray.h"` and any `#include <GL/...>`/`<GLEW/...>`. Add:

```cpp
#include "Core/Components/Material.h"
#include "Core/Components/Transform.h"
#include "Rendering/IndexBuffer.h"
#include "Rendering/UniformBuffer.h"
#include "Rendering/VertexBuffer.h"
```

**Edit 2 — shader IDs.** Replace the shader-ID enum with:

```cpp
enum class AnimationShaderID : int32_t
{
	NONE = 0,
	DRAWING,
	MODEL_DRAW,
};
```

and the registry call with:

```cpp
ShaderRegistry::Instance()->Init("res/Shaders/", [](ShaderRegistry* reg) {
	reg->RegisterShader(static_cast<int32_t>(AnimationShaderID::DRAWING),
						"Drawing.vert", "Drawing.frag",
						"spirv/Drawing.vert.spv", "spirv/Drawing.frag.spv");
	reg->RegisterShader(static_cast<int32_t>(AnimationShaderID::MODEL_DRAW),
						"ModelDraw.vert", "ModelDraw.frag",
						"spirv/ModelDraw.vert.spv", "spirv/ModelDraw.frag.spv");
});
```

**Edit 3 — `Init()`.** Replace every `glEnable`/`glCullFace`/`SetUniform*`/`CreateVertexArray` block with:

```cpp
	mCameraUBO = Renderer::Instance()->CreateUniformBuffer();
	mCameraUBO->Init(sizeof(CameraMatricesUBO), 2);

	mLightUBO = Renderer::Instance()->CreateUniformBuffer();
	mLightUBO->Init(sizeof(LightDataUBO), 3);

	mBoneUBO = Renderer::Instance()->CreateUniformBuffer();
	mBoneUBO->Init(sizeof(glm::mat4) * 128, 4);

	// Padding so a short skeleton still fills the whole 128-entry array.
	mBoneMatrices.assign(128, glm::mat4(1.0f));

	Renderer::Instance()->SetClearColor(glm::vec3(0.2f, 0.2f, 0.2f));
	Renderer::Instance()->SetDepthTest(true);
	Renderer::Instance()->SetCullingFace(CullFace::Front);
```

`CameraMatricesUBO`/`LightDataUBO` are the structs from section 2.5; declare them in the same anonymous namespace as the shader enum.

**Edit 4 — `Update()`.** Replace the old draw section with this shape:

```cpp
	Camera* camera = Engine::Instance()->GetCamera();

	CameraMatricesUBO cameraData{ camera->mProjection, camera->mView, glm::inverse(camera->mView) };
	mCameraUBO->AddData(sizeof(cameraData), &cameraData);

	LightDataUBO lightData{ glm::vec4(mLight->position, 1.0f), glm::vec4(1.0f) };
	mLightUBO->AddData(sizeof(lightData), &lightData);

	UploadBones();

	Renderer::Instance()->BindUniformBuffer(mCameraUBO, 2);
	Renderer::Instance()->BindUniformBuffer(mLightUBO, 3);
	Renderer::Instance()->BindUniformBuffer(mBoneUBO, 4);

	mDrawIndex = 0;

	// --- floor ----------------------------------------------------------
	{
		DrawUniforms& uniforms = NextDrawUniforms();

		const glm::mat4 model = glm::scale(glm::mat4(1.0f), glm::vec3(20.0f, 1.0f, 20.0f));
		ObjectMatricesUBO objectData{ glm::inverse(model), model, model };
		uniforms.mObjectUBO->AddData(sizeof(objectData), &objectData);

		AnimationMaterialUBO material{};
		material.mDiffuse = glm::vec4(mFloorColor, 1.0f);
		material.mSpecular = glm::vec4(0.0f);
		material.mShininess = 1.0f;
		material.mLighting = 1;
		material.mUseTexture = 0;
		uniforms.mMaterialUBO->AddData(sizeof(material), &material);

		Renderer::Instance()->BindUniformBuffer(uniforms.mObjectUBO, 0);
		Renderer::Instance()->BindUniformBuffer(uniforms.mMaterialUBO, 1);
		Renderer::Instance()->Draw(mFloorVBO, mFloorIBO, mDrawingShader);
	}

	// --- skinned model --------------------------------------------------
	{
		DrawUniforms& uniforms = NextDrawUniforms();

		const glm::mat4 model = mModelTransform;   // keep the existing model matrix
		ObjectMatricesUBO objectData{ glm::inverse(model), model, model };
		uniforms.mObjectUBO->AddData(sizeof(objectData), &objectData);

		AnimationMaterialUBO material{};
		material.mDiffuse = glm::vec4(mArcherColor, 1.0f);
		material.mSpecular = glm::vec4(0.0f);
		material.mShininess = 1.0f;
		material.mLighting = 1;
		material.mUseTexture = 1;
		uniforms.mMaterialUBO->AddData(sizeof(material), &material);

		Renderer::Instance()->BindUniformBuffer(uniforms.mObjectUBO, 0);
		Renderer::Instance()->BindUniformBuffer(uniforms.mMaterialUBO, 1);
		mModel.Draw(mModelShader);   // Mesh::Draw binds textures at slot 8+i (P2-00)
	}

	// --- curve polyline -------------------------------------------------
	{
		DrawUniforms& uniforms = NextDrawUniforms();

		const glm::mat4 model(1.0f);
		ObjectMatricesUBO objectData{ model, model, model };
		uniforms.mObjectUBO->AddData(sizeof(objectData), &objectData);

		AnimationMaterialUBO material{};
		material.mDiffuse = glm::vec4(1.0f, 0.2f, 0.2f, 1.0f);
		material.mLighting = 0;
		material.mUseTexture = 0;
		uniforms.mMaterialUBO->AddData(sizeof(material), &material);

		Renderer::Instance()->BindUniformBuffer(uniforms.mObjectUBO, 0);
		Renderer::Instance()->BindUniformBuffer(uniforms.mMaterialUBO, 1);
		Renderer::Instance()->DebugDrawLines(mCurveVBO, mCurveIBO, mDrawingShader);
	}
```

**Edit 5 — `UploadBones()`.** Add:

```cpp
void AnimationScene::UploadBones()
{
	// Fill the first N entries with the pose, keep the rest identity.
	const size_t boneCount = std::min(mBoneMatrices.size(), static_cast<size_t>(128));

	for (size_t i = 0; i < boneCount; ++i)
	{
		mBoneMatrices[i] = mAnimation.GetBoneMatrix(i);   // existing accessor
	}

	for (size_t i = boneCount; i < 128; ++i)
	{
		mBoneMatrices[i] = glm::mat4(1.0f);
	}

	mBoneUBO->AddData(sizeof(glm::mat4) * 128, mBoneMatrices.data());
}
```

> **Mismatch check:** the old code obtains bone matrices some other way (a `std::vector<glm::mat4>` on the `Animation` object, a `mSkeleton` array, or per-bone `GetTransform()` calls). Whatever the source, the loop body must produce the **final** skinning matrix for bone `i` (inverse bind pose already applied) — that is what `ModelDraw.vert` multiplies into the vertex. If the existing code uploads the same matrices through `SetUniformMat4Array`, copy that expression verbatim into the loop.

**Edit 6 — `BuildCurveBuffers()`.** Add:

```cpp
void AnimationScene::BuildCurveBuffers()
{
	// The curve is a polyline: N control points produce N-1 line segments, each
	// a pair of indices. DebugDrawLines draws GL_LINES / VK_LINE_LIST.
	std::vector<glm::vec3> points = mAnimation.GetCurvePoints();   // existing accessor

	if (points.size() < 2)
	{
		return;
	}

	std::vector<uint16_t> indices;
	indices.reserve((points.size() - 1) * 2);

	for (size_t i = 0; i + 1 < points.size(); ++i)
	{
		indices.push_back(static_cast<uint16_t>(i));
		indices.push_back(static_cast<uint16_t>(i + 1));
	}

	if (mCurveVBO == nullptr)
	{
		mCurveVBO = Renderer::Instance()->CreateVertexBuffer();
		mCurveVBO->SetDataLayout({ VertexFormat::Vec3, VertexFormat::Vec3 });
		mCurveIBO = Renderer::Instance()->CreateIndexBuffer();
	}

	mCurveVBO->AddDynamicData(points.data(), static_cast<unsigned int>(points.size() * sizeof(glm::vec3) * 2));
	mCurveIBO->AddData(indices.data(), static_cast<unsigned int>(indices.size()), IndexType::UInt16);
}
```

**Important:** the layout is `{ Vec3, Vec3 }` because `Drawing.vert` declares locations 0, 1 and 2. A vertex buffer with only one `Vec3` would leave `aNormal` and `aTexCoord` undefined. Upload each point twice — once as position, once as a dummy normal — or build an interleaved `struct { glm::vec3 p; glm::vec3 n; }` array. Use the interleaved array; the `AddDynamicData` above is only correct for an interleaved source buffer. Correct version:

```cpp
	struct CurveVertex
	{
		glm::vec3 mPosition;
		glm::vec3 mNormal;
	};

	std::vector<CurveVertex> vertices;
	vertices.reserve(points.size());
	for (const glm::vec3& point : points)
	{
		vertices.push_back({ point, glm::vec3(0.0f, 0.0f, 1.0f) });
	}

	mCurveVBO->AddDynamicData(vertices.data(),
							  static_cast<unsigned int>(vertices.size() * sizeof(CurveVertex)));
```

**Edit 7 — `Close()`.** Delete the shaders, the four UBOs, the curve buffers and every entry of `mDrawUniforms`, then set the pointers to `nullptr` and clear the vector. `~AnimationScene()` must call `Close()`.

**Edit 8 — `RenderObject()`.** Leave the body empty; this scene draws directly.

**Verify:**

```powershell
cmake --build --preset OpenGL-Debug --target AnimationTechniques --parallel
```

Expected: exit `0`.

**Stop if:** `Mesh::Draw(Shader*)` does not exist. Check `Assets/Mesh.h` for the exact signature; if it is `Draw(Shader*, ...)` with extra arguments, call it with the same arguments the old code used, and **report the signature in the commit message**.

**Commit:** with P2-05.

### P2-04 — Rewrite `Mechanics`

**Files:** `GraphicsFramework/AnimationTechniques/src/Mechanics.h`, `GraphicsFramework/AnimationTechniques/src/Mechanics.cpp`

`Mechanics` simulates a rope of cubes hanging between two spheres. It has the same rendering problems as `AnimationScene` plus one more: **one transform UBO per rope node**.

**Design:**

- One shared `MaterialData` UBO per *material class* (cubes, spheres) — two objects total, uploaded once.
- One `ObjectMatricesUBO` **per rope node**, allocated in `Init()` once the node count is known, plus one for each sphere.
- One shared `CameraMatricesUBO` and one `LightDataUBO`.
- Node transforms are written every frame into their own UBO — never shared.

**`Mechanics.h` — required members:**

```cpp
private:
	struct DrawUniforms
	{
		UniformBuffer* mObjectUBO = nullptr;
		UniformBuffer* mMaterialUBO = nullptr;
	};

	Light* mLight = nullptr;
	Shader* mShader = nullptr;

	UniformBuffer* mCameraUBO = nullptr;
	UniformBuffer* mLightUBO = nullptr;

	std::vector<DrawUniforms> mNodeUniforms;   // one entry per rope node
	DrawUniforms mLeftSphere;
	DrawUniforms mRightSphere;

	VertexBuffer* mCubeVBO = nullptr;
	IndexBuffer* mCubeIBO = nullptr;
	VertexBuffer* mSphereVBO = nullptr;
	IndexBuffer* mSphereIBO = nullptr;
```

**`Mechanics.cpp` — required changes:**

1. Delete `VertexArray`, `glEnable`, `glCullFace`, `SetUniform*`.
2. In `Init()`, after the node count is known (`mNodes.size()`), do:

```cpp
	mNodeUniforms.resize(mNodes.size());
	for (DrawUniforms& uniforms : mNodeUniforms)
	{
		uniforms.mObjectUBO = Renderer::Instance()->CreateUniformBuffer();
		uniforms.mObjectUBO->Init(sizeof(ObjectMatricesUBO), 0);

		uniforms.mMaterialUBO = Renderer::Instance()->CreateUniformBuffer();
		uniforms.mMaterialUBO->Init(sizeof(AnimationMaterialUBO), 1);
	}
```

3. Build the cube and sphere geometry with `ShapeManager` (`MakeVAO` pattern) or `ObjectManager` JSON, whichever the current file uses — but the result must be a `VertexBuffer` + `IndexBuffer` pair, and the index buffer must be `IndexType::UInt16`.
4. In `Update()`, per node:

```cpp
	ObjectMatricesUBO objectData{ glm::inverse(model), model, model };
	uniform.mObjectUBO->AddData(sizeof(objectData), &objectData);
	Renderer::Instance()->BindUniformBuffer(uniform.mObjectUBO, 0);
	Renderer::Instance()->BindUniformBuffer(uniform.mMaterialUBO, 1);
	Renderer::Instance()->Draw(mCubeVBO, mCubeIBO, mShader);
```

5. `SetClearColor(glm::vec3(0.2f, 0.2f, 0.2f))`, `SetDepthTest(true)`, `SetCullingFace(CullFace::Back)`.
6. `Close()` deletes everything and nulls it; `~Mechanics()` calls `Close()`.

**Verify:**

```powershell
cmake --build --preset OpenGL-Debug --target AnimationTechniques --parallel
cmake --build --preset Vulkan-Debug --target AnimationTechniques --parallel
```

Expected: both exit `0`.

**Commit:** with P2-05.

### P2-05 — Gate: `AnimationTechniques` renders both scenes on both backends

```powershell
cmake --build --preset OpenGL-Debug   --target AnimationTechniques --parallel
cmake --build --preset OpenGL-Release --target AnimationTechniques --parallel
cmake --build --preset Vulkan-Debug   --target AnimationTechniques --parallel
cmake --build --preset Vulkan-Release --target AnimationTechniques --parallel
```

```powershell
powershell -ExecutionPolicy Bypass -File scripts\verify-scene.ps1 -Preset OpenGL-Debug -BuildTarget AnimationTechniques -Scene 1 -Background "51,51,51" -SettleMs 3000
powershell -ExecutionPolicy Bypass -File scripts\verify-scene.ps1 -Preset Vulkan-Debug -BuildTarget AnimationTechniques -Scene 1 -Background "51,51,51" -SettleMs 3000
powershell -ExecutionPolicy Bypass -File scripts\verify-scene.ps1 -Preset OpenGL-Debug -BuildTarget AnimationTechniques -Scene 2 -Background "51,51,51" -SettleMs 3000
powershell -ExecutionPolicy Bypass -File scripts\verify-scene.ps1 -Preset Vulkan-Debug -BuildTarget AnimationTechniques -Scene 2 -Background "51,51,51" -SettleMs 3000
```

**Acceptance:**

| Run | Must contain |
|---|---|
| `-Scene 1` | The archer model at the centre, a green floor plane, a red polyline. `content >= 3%`. |
| `-Scene 2` | A chain of red cubes between two spheres. `content >= 2%`. |

All four: `RESULT : PASS`, `VALIDATION : no errors in stdout/stderr`.

**Scene 1 is the risky one.** If the model is invisible but the floor renders, the bone UBO is the first suspect: the skinning matrix must be the *final* one (bind-pose inverse already applied), and `aBoneIDs` must be valid indices into the 128-entry array. If the model renders but is flattened into the floor, `Object.model` is not being applied twice — check `ModelDraw.vert` multiplies `Object.model * skinnedPosition` exactly once.

**Commit:** `feat(animation): port AnimationScene and Mechanics to the RHI (per-draw UBO pools, bone UBO, line buffers)`
