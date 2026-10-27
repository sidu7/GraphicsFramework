---

## 8. Phase 3 — `RealTimeRayTracing`

**Project root:** `GraphicsFramework/RealTimeRayTracing/`
**Track:** A.
**Strategy:** the existing sample is an **OpenGL compute-shader** ray tracer (`TraceRay.comp` writing into an SSBO, then a fullscreen pass that re-uploads the SSBO through `glTexImage2D`). None of that is portable:

| Old piece | Why it cannot be ported as-is |
|---|---|
| `ComputeShader("res/shaders/TraceRay.comp")` | `#version 430 compatibility` + `#extension` directives; `glslc` rejects it. `ComputeShaderVK` is a stub. |
| `ShaderStorageBuffer` + `gl_GlobalInvocationID` | `ShaderStorageBufferVK` exists but `ComputeShaderVK::SetUniform*` are stubs (V10). |
| `glGenTextures` / `glTexImage2D` / `glGenerateMipmap` | GL-only calls. |
| `Engine::Instance().pCamera`, `scrWidth`, `scrHeight` | Removed APIs. |
| `Camera::mPrevView` in the draw shader | Not uploaded anywhere any more. |
| `Shader(path, path)` | Removed constructor. |

**Replacement:** a **fragment-shader ray tracer** on a fullscreen quad. The whole scene is analytic (one sphere, one ground plane, one directional-ish point light), so a fragment shader is sufficient, needs no compute, no SSBO, no offscreen target, and runs identically on both backends.

Visual target (also the acceptance criterion): a red sphere sitting on a grey ground plane, a hard shadow under it, and a blue sky gradient. Clear colour `(0.05, 0.07, 0.12)`.

### P3-00 — Make `Renderer::DrawQuad` Vulkan-safe (framework)

**Why:** `Renderer::DrawQuad` creates a `VertexBuffer`/`IndexBuffer`, records a draw, then `delete`s both. On Vulkan the draw is only *recorded* into the command buffer; the buffer is destroyed before the frame is submitted, so the GPU (and the validation layer) sees a destroyed `VkBuffer` (`VUID-vkDestroyBuffer-buffer-00922`). It also allocates two buffers per call, which matters once ART calls it for every fullscreen pass.

**Files:** `GraphicsFramework/GraphicsFramework/Source/Rendering/Renderer.cpp`

**Replace the whole `Renderer::DrawQuad` body** (keep the signature) with:

```cpp
void Renderer::DrawQuad(const Shader* shader)
{
	// The quad geometry never changes, so it is created once per process.
	// It is intentionally never deleted: on Vulkan the command buffer keeps
	// referencing the vertex buffer until the frame is submitted, so a
	// create/delete per call would destroy a buffer that is still in use.
	static VertexBuffer* VBO_quad = nullptr;
	static IndexBuffer* IBO_quad = nullptr;

	if (VBO_quad == nullptr)
	{
		// NDC position (location 0) + texture coordinate (location 1).
		const float positions_quad[] = {
				-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
				 1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
				 1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
				-1.0f,  1.0f, 0.0f, 0.0f, 1.0f
		};

		const uint16_t indices_quad[] = {
			0, 1, 2,
			2, 3, 0
		};

		VBO_quad = Renderer::Instance()->CreateVertexBuffer();
		IBO_quad = Renderer::Instance()->CreateIndexBuffer();

		VBO_quad->SetDataLayout({ VertexFormat::Vec3, VertexFormat::Vec2 });
		VBO_quad->AddData(positions_quad, sizeof(positions_quad));
		IBO_quad->AddData(indices_quad, 6, IndexType::UInt16);
	}

	Draw(VBO_quad, IBO_quad, shader);
}
```

**Notes:**

- The static pointers are deliberately leaked. Deleting them at static-destruction time would run after `Renderer::Close()` has destroyed the device.
- `sizeof(positions_quad)` replaces the old `5 * 4 * sizeof(float)`, which is the same value but not tied to the array length.

**Verify:**

```powershell
cmake --build --preset Vulkan-Debug --target UnitTests --parallel
cmake --build --preset OpenGL-Debug --target UnitTests --parallel
```

Expected: both exit `0`. No UnitTests scene uses `DrawQuad`, so there is nothing else to verify yet; the RTT gate in P3-06 proves the change.

**Commit:** `fix(renderer): keep the DrawQuad geometry alive for the whole process`

---

### P3-01 — Delete the compute path

**Files to delete:**

```
GraphicsFramework/RealTimeRayTracing/src/Shapes.h
GraphicsFramework/RealTimeRayTracing/res/shaders/TraceRay.comp
GraphicsFramework/RealTimeRayTracing/res/shaders/Shapes.comp
GraphicsFramework/RealTimeRayTracing/res/shaders/Drawing.vert
GraphicsFramework/RealTimeRayTracing/res/shaders/Drawing.frag
GraphicsFramework/RealTimeRayTracing/res/shaders/raytraceout.vert
GraphicsFramework/RealTimeRayTracing/res/shaders/raytraceout.frag
GraphicsFramework/RealTimeRayTracing/res/shaders/spirv/            (whole folder)
```

**Command:**

```powershell
Remove-Item -Force `
  GraphicsFramework\RealTimeRayTracing\src\Shapes.h, `
  GraphicsFramework\RealTimeRayTracing\res\shaders\TraceRay.comp, `
  GraphicsFramework\RealTimeRayTracing\res\shaders\Shapes.comp, `
  GraphicsFramework\RealTimeRayTracing\res\shaders\Drawing.vert, `
  GraphicsFramework\RealTimeRayTracing\res\shaders\Drawing.frag, `
  GraphicsFramework\RealTimeRayTracing\res\shaders\raytraceout.vert, `
  GraphicsFramework\RealTimeRayTracing\res\shaders\raytraceout.frag
Remove-Item -Recurse -Force GraphicsFramework\RealTimeRayTracing\res\shaders\spirv
```

**Keep:** `res/data/ball.json`. It is no longer loaded (the sphere is analytic), but it documents the sphere's material values that `RayTracing.cpp` hardcodes. Do not delete data files.

**Why delete instead of port:** `add_framework_spirv` globs `*.comp` and runs `glslc` on every configuration. Leaving `TraceRay.comp` in the tree keeps the **OpenGL** build red forever.

**Verify:**

```powershell
Get-ChildItem -Recurse -File GraphicsFramework\RealTimeRayTracing\res\shaders | Select-Object Name
```

Expected: only `Fullscreen.vert` and `RayTracing.frag` after P3-02. At this point the folder may be empty.

**Commit:** with P3-06.

---

### P3-02 — New shaders

**Files (new):** `res/shaders/Fullscreen.vert`, `res/shaders/RayTracing.frag`

#### `Fullscreen.vert`

```glsl
#version 450

// Matches Renderer::DrawQuad's vertex layout: location 0 = NDC position,
// location 1 = texture coordinate with v = 0 at the bottom of the quad.
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

layout (location = 0) out vec2 vTexCoord;

void main()
{
	vTexCoord = aTexCoord;
	gl_Position = vec4(aPos, 1.0);
}
```

#### `RayTracing.frag`

```glsl
#version 450

layout (location = 0) in vec2 vTexCoord;

layout (location = 0) out vec4 FragColor;

// Per-frame camera, uploaded by the scene (binding 2)
layout (binding = 2) uniform CameraMatrices
{
	mat4 projection;
	mat4 view;
	mat4 invView;
} Camera;

// Scene description (binding 4). Layout mirrors RayTracingParamsUBO in
// src/RayTracing.cpp exactly: nine vec4s, 144 bytes.
layout (binding = 4) uniform RayTracingParams
{
	vec4 eye;          // xyz = camera position
	vec4 lightPos;     // xyz = light position, w = light intensity
	vec4 lightColor;   // rgb = light colour
	vec4 sphere;       // xyz = centre, w = radius
	vec4 sphereColor;  // rgb = albedo, w = shininess (unused, reserved)
	vec4 groundColor;  // rgb = albedo, w = ground height (y)
	vec4 skyTop;       // rgb = sky colour looking up
	vec4 skyBottom;    // rgb = sky colour looking at the horizon
	vec4 params;       // x = ambient, y = specular strength, zw unused
} Params;

const float Epsilon = 1e-4;

// Ray / sphere. Returns the nearest positive t, or -1.0 when there is no hit.
float HitSphere(vec3 origin, vec3 dir, vec3 centre, float radius)
{
	vec3 oc = origin - centre;
	float b = dot(oc, dir);
	float c = dot(oc, oc) - radius * radius;
	float discriminant = b * b - c;

	if (discriminant < 0.0)
	{
		return -1.0;
	}

	float root = sqrt(discriminant);
	float t0 = -b - root;
	float t1 = -b + root;

	if (t0 > Epsilon)
	{
		return t0;
	}
	if (t1 > Epsilon)
	{
		return t1;
	}

	return -1.0;
}

// Ray / horizontal plane at y = height. Returns t, or -1.0.
float HitPlane(vec3 origin, vec3 dir, float height)
{
	if (abs(dir.y) < Epsilon)
	{
		return -1.0;
	}

	float t = (height - origin.y) / dir.y;
	return t > Epsilon ? t : -1.0;
}

vec3 SkyColor(vec3 dir)
{
	float t = clamp(dir.y * 0.5 + 0.5, 0.0, 1.0);
	return mix(Params.skyBottom.rgb, Params.skyTop.rgb, t);
}

// True when the segment from P towards L is blocked by the sphere.
bool InShadow(vec3 P, vec3 L)
{
	float t = HitSphere(P, L, Params.sphere.xyz, Params.sphere.w);
	return t > Epsilon;
}

void main()
{
	// Reconstruct the view ray from the clip-space position that this fragment
	// was rasterised at. Using the interpolated texture coordinate (and not
	// gl_FragCoord) keeps the image upright on both backends: Camera.projection
	// already contains Renderer::GetProjectionYFlip(), which is -1 on Vulkan.
	vec2 ndc = vTexCoord * 2.0 - 1.0;

	vec4 viewFar = inverse(Camera.projection) * vec4(ndc, 1.0, 1.0);
	viewFar /= viewFar.w;

	vec3 dir = normalize(mat3(Camera.invView) * viewFar.xyz);
	vec3 origin = Params.eye.xyz;

	float tSphere = HitSphere(origin, dir, Params.sphere.xyz, Params.sphere.w);
	float tPlane = HitPlane(origin, dir, Params.groundColor.w);

	// Nothing hit: sky gradient.
	if (tSphere < 0.0 && tPlane < 0.0)
	{
		FragColor = vec4(SkyColor(dir), 1.0);
		return;
	}

	// Nearest hit wins.
	bool hitSphere = (tSphere > 0.0) && (tPlane < 0.0 || tSphere < tPlane);
	float t = hitSphere ? tSphere : tPlane;

	vec3 P = origin + t * dir;
	vec3 N;
	vec3 albedo;

	if (hitSphere)
	{
		N = normalize(P - Params.sphere.xyz);
		albedo = Params.sphereColor.rgb;
	}
	else
	{
		N = vec3(0.0, 1.0, 0.0);
		albedo = Params.groundColor.rgb;

		// Fade the ground into the horizon so the plane has no hard edge.
		float fade = clamp(1.0 - t / 60.0, 0.0, 1.0);
		albedo = mix(SkyColor(dir), albedo, fade);
	}

	vec3 L = normalize(Params.lightPos.xyz - P);
	float NL = max(dot(N, L), 0.0);

	float shadow = 1.0;
	if (NL > 0.0)
	{
		// Offset along the normal so the shadow ray does not hit the surface itself.
		shadow = InShadow(P + N * (Epsilon * 100.0), L) ? 0.0 : 1.0;
	}

	float ambient = Params.params.x;
	float intensity = Params.lightPos.w;

	vec3 color = albedo * (ambient + (1.0 - ambient) * NL * shadow * intensity);
	color *= Params.lightColor.rgb;

	// Small Blinn-Phong highlight so the sphere reads as a sphere.
	vec3 V = normalize(Params.eye.xyz - P);
	vec3 H = normalize(L + V);
	float specular = pow(max(dot(N, H), 0.0), Params.sphereColor.w) * Params.params.y * shadow;

	FragColor = vec4(color + vec3(specular), 1.0);
}
```

**Notes:**

- `inverse()` is a GLSL 4.50 built-in. Do not replace it with a hand-written inverse; the projection matrix is standard.
- `Params.groundColor.w` carries the **ground height** (`-1.0`). It is a `vec4` member so it stays std140-aligned; reusing `w` avoids adding another 16-byte slot.
- `HitPlane` returns a hit for **any** direction with a downward or upward component, including rays that hit behind the camera; the `t > Epsilon` test rejects those.
- The shadow ray uses a `1e-2` normal offset. Smaller values self-shadow the sphere (speckled shadow terminator); larger values detach the shadow.

**Verify:**

```powershell
& "C:\VulkanSDK\1.4.328.1\Bin\glslc.exe" --target-env=vulkan1.2 GraphicsFramework\RealTimeRayTracing\res\shaders\Fullscreen.vert -o $env:TEMP\rtt_v.spv
& "C:\VulkanSDK\1.4.328.1\Bin\glslc.exe" --target-env=vulkan1.2 GraphicsFramework\RealTimeRayTracing\res\shaders\RayTracing.frag -o $env:TEMP\rtt_f.spv
```

Expected: two silent successes.

**Stop if:** `glslc` reports `'inverse' : no matching overloaded function found`. That means the shader declares `#version 450` with a lower effective profile; check the first line is exactly `#version 450`.

**Commit:** with P3-06.

---

### P3-03 — `res/Settings.json`

**Files (new):** `GraphicsFramework/RealTimeRayTracing/res/Settings.json`

```json
{
  "WindowSize": [ 1280.0, 720.0 ],
  "TransformBindingPoint": 0,
  "MaterialBindingPoint": 1
}
```

The old sample ran at 400×300; 1280×720 matches the other samples and makes the sphere readable in the screenshot.

**Commit:** with P3-06.

---

### P3-04 — Rewrite `RayTracing.h`

**Files:** `GraphicsFramework/RealTimeRayTracing/src/RayTracing.h`

**Replace the whole file:**

```cpp
#pragma once
#include <core/Scene.h>
#include <glm/glm.hpp>

class Light;
class Shader;
class UniformBuffer;

// Scene description uploaded once per frame to binding 4.
// Must match layout(binding = 4) uniform RayTracingParams in RayTracing.frag:
// nine vec4s, 144 bytes, std140-aligned.
struct RayTracingParamsUBO
{
	glm::vec4 mEye;         // xyz = camera position
	glm::vec4 mLightPos;    // xyz = light position, w = intensity
	glm::vec4 mLightColor;  // rgb = light colour
	glm::vec4 mSphere;      // xyz = centre, w = radius
	glm::vec4 mSphereColor; // rgb = albedo, w = shininess
	glm::vec4 mGroundColor; // rgb = albedo, w = ground height (y)
	glm::vec4 mSkyTop;      // rgb
	glm::vec4 mSkyBottom;   // rgb
	glm::vec4 mParams;      // x = ambient, y = specular strength
};

class RayTracing : public Scene
{
public:
	RayTracing() = default;
	~RayTracing() override;

	void Init() override;
	void Close() override;
	void Update() override;
	void RenderObject(class Object* object, class Shader* shader) override;
	void DebugDisplay() override;

private:
	Light* mLight = nullptr;
	Shader* mRayTracingShader = nullptr;

	UniformBuffer* mCameraUBO = nullptr;  // binding 2
	UniformBuffer* mParamsUBO = nullptr;  // binding 4

	float mAmbient = 0.15f;
	float mSpecularStrength = 0.35f;
};
```

**Notes:**

- `Shapes.h` is gone; do not include it.
- `ComputeShader`, `ShaderStorageBuffer` and the `RShapes` types are gone with it.
- The two tunables are `float` members so `DebugDisplay` can expose them with sliders.

**Commit:** with P3-05.

---

### P3-05 — Rewrite `RayTracing.cpp`

**Files:** `GraphicsFramework/RealTimeRayTracing/src/RayTracing.cpp`

**Replace the whole file:**

```cpp
#include "RayTracing.h"

#include <cstdint>
#include <iostream>

#include "Core/Camera.h"
#include "Core/Engine.h"
#include "Core/Light.h"
#include "Rendering/Renderer.h"
#include "Rendering/Shader.h"
#include "Rendering/ShaderRegistry.h"
#include "Rendering/UniformBuffer.h"

#include "Imgui/imgui.h"

namespace
{
	constexpr uint32_t CameraBindingPoint = 2;
	constexpr uint32_t ParamsBindingPoint = 4;

	enum class RayTracingShaderID : int32_t
	{
		NONE = 0,
		RAY_TRACING,
	};

	struct CameraMatricesUBO
	{
		glm::mat4 mProjection;
		glm::mat4 mView;
		glm::mat4 mInverseView;
	};

	// Values mirrored from res/data/ball.json (Diffuse, Specular, Shininess,
	// Scale) so the analytic sphere matches the asset the old sample loaded.
	constexpr float SphereRadius = 1.0f;
	constexpr float GroundHeight = -1.0f;
}

RayTracing::~RayTracing()
{
	Close();
}

void RayTracing::Close()
{
	delete mLight;
	mLight = nullptr;

	delete mRayTracingShader;
	mRayTracingShader = nullptr;

	delete mCameraUBO;
	mCameraUBO = nullptr;

	delete mParamsUBO;
	mParamsUBO = nullptr;
}

void RayTracing::Init()
{
	std::cout << "Real Time Ray Tracing" << std::endl;

	ShaderRegistry::Instance()->Init("res/shaders/", [](ShaderRegistry* reg) {
		reg->RegisterShader(static_cast<int32_t>(RayTracingShaderID::RAY_TRACING),
							"Fullscreen.vert", "RayTracing.frag",
							"spirv/Fullscreen.vert.spv", "spirv/RayTracing.frag.spv");
	});

	// Same light position the old sample used.
	mLight = new Light(glm::vec3(10.0f, 100.0f, 40.0f));

	Camera* camera = Engine::Instance()->GetCamera();
	camera->mCameraPos = glm::vec3(-10.0f, 7.0f, 16.0f);
	camera->pitch = -19.0f;
	camera->yaw = -56.4f;
	camera->mSpeed *= 2.0f;
	camera->CalculateFront();

	mRayTracingShader = Renderer::Instance()->CreateShader();
	mRayTracingShader->Init(static_cast<int32_t>(RayTracingShaderID::RAY_TRACING));

	mCameraUBO = Renderer::Instance()->CreateUniformBuffer();
	mCameraUBO->Init(sizeof(CameraMatricesUBO), CameraBindingPoint);

	mParamsUBO = Renderer::Instance()->CreateUniformBuffer();
	mParamsUBO->Init(sizeof(RayTracingParamsUBO), ParamsBindingPoint);

	Renderer::Instance()->SetClearColor(glm::vec3(0.05f, 0.07f, 0.12f));
	// The quad covers every pixel and is drawn at clip z = 0, so depth testing
	// only adds a chance of the quad being rejected.
	Renderer::Instance()->SetDepthTest(false);
	Renderer::Instance()->SetCullingFace(CullFace::None);
}

void RayTracing::Update()
{
	Camera* camera = Engine::Instance()->GetCamera();

	CameraMatricesUBO cameraData{ camera->mProjection, camera->mView, glm::inverse(camera->mView) };
	mCameraUBO->AddData(sizeof(cameraData), &cameraData);

	RayTracingParamsUBO params{};
	params.mEye = glm::vec4(camera->mCameraPos, 1.0f);
	params.mLightPos = glm::vec4(mLight->position, 1.0f);
	params.mLightColor = glm::vec4(1.0f, 0.97f, 0.9f, 1.0f);
	params.mSphere = glm::vec4(0.0f, 0.0f, 0.0f, SphereRadius);
	params.mSphereColor = glm::vec4(1.0f, 0.1f, 0.1f, 64.0f);
	params.mGroundColor = glm::vec4(0.35f, 0.35f, 0.38f, GroundHeight);
	params.mSkyTop = glm::vec4(0.05f, 0.10f, 0.28f, 1.0f);
	params.mSkyBottom = glm::vec4(0.62f, 0.70f, 0.85f, 1.0f);
	params.mParams = glm::vec4(mAmbient, mSpecularStrength, 0.0f, 0.0f);
	mParamsUBO->AddData(sizeof(params), &params);

	Renderer::Instance()->BindUniformBuffer(mCameraUBO, CameraBindingPoint);
	Renderer::Instance()->BindUniformBuffer(mParamsUBO, ParamsBindingPoint);

	// DrawQuad does not bind the shader on Vulkan (Device::Draw allocates the
	// descriptor set from mBoundShader), so bind it explicitly.
	Renderer::Instance()->BindShader(mRayTracingShader);
	Renderer::Instance()->DrawQuad(mRayTracingShader);
}

void RayTracing::RenderObject(Object* object, Shader* shader)
{
	// The image is produced by the fullscreen ray tracing pass.
}

void RayTracing::DebugDisplay()
{
	ImGui::SetNextWindowPos(ImVec2(330.0f, 40.0f), ImGuiCond_FirstUseEver);
	ImGui::Begin("Ray Tracing");
	ImGui::SliderFloat("Ambient", &mAmbient, 0.0f, 1.0f);
	ImGui::SliderFloat("Specular", &mSpecularStrength, 0.0f, 2.0f);
	ImGui::Text("Sphere radius: %.2f", SphereRadius);
	ImGui::Text("Camera: %.1f %.1f %.1f",
				Engine::Instance()->GetCamera()->mCameraPos.x,
				Engine::Instance()->GetCamera()->mCameraPos.y,
				Engine::Instance()->GetCamera()->mCameraPos.z);
	ImGui::End();
}
```

**Notes:**

- **Do not** call `Renderer::Clear()`. Both backends clear at `StartFrame`.
- **Do not** read `Engine::Instance()->scrWidth` / `scrHeight`; they no longer exist. The ray reconstruction uses the camera matrices, so the renderer never needs to know the viewport size.
- `SphereRadius` is a `constexpr float` in an anonymous namespace. Referencing it from `DebugDisplay` is legal.
- `mSpecularStrength` multiplies `pow(N·H, shininess)`, **not** a Fresnel term; this is deliberately simple.

**Verify:**

```powershell
cmake --build --preset OpenGL-Debug --target RealTimeRayTracing --parallel
```

**Expected errors (and only these):**

| Error | Cause |
|---|---|
| `C2065 'RShapes': undeclared identifier` | `Shapes.h` was not deleted, or `RayTracing.h` still includes it. |
| `C2039 'pCamera'` | `Engine::Instance().pCamera` left in the file. |
| `C2660 'Renderer::DrawQuad': function does not take 0 arguments` | the old `DrawQuad()` call was kept. |

**Commit:** `feat(rtt): replace the GL compute ray tracer with a portable fragment-shader ray tracer`

---

### P3-06 — `Main.cpp` and gate

**Files:** `GraphicsFramework/RealTimeRayTracing/src/Main.cpp`

**Replace the whole file:**

```cpp
#include <iostream>

#include "Core/Engine.h"
#include "RayTracing.h"

int main(int argc, char* args[])
{
	std::cout << "Real Time Ray Tracing" << std::endl;

	Engine::Instance()->Start("res/Settings.json");
	Engine::Instance()->SetScene<RayTracing>();
	Engine::Instance()->Run();
	Engine::Instance()->Stop();

	return 0;
}
```

**Build all four presets:**

```powershell
cmake --build --preset OpenGL-Debug   --target RealTimeRayTracing --parallel
cmake --build --preset OpenGL-Release --target RealTimeRayTracing --parallel
cmake --build --preset Vulkan-Debug   --target RealTimeRayTracing --parallel
cmake --build --preset Vulkan-Release --target RealTimeRayTracing --parallel
```

**Verify all four:**

```powershell
powershell -ExecutionPolicy Bypass -File scripts\verify-scene.ps1 -Preset OpenGL-Debug   -BuildTarget RealTimeRayTracing -NoScene -Background "13,18,31"
powershell -ExecutionPolicy Bypass -File scripts\verify-scene.ps1 -Preset OpenGL-Release -BuildTarget RealTimeRayTracing -NoScene -Background "13,18,31"
powershell -ExecutionPolicy Bypass -File scripts\verify-scene.ps1 -Preset Vulkan-Debug   -BuildTarget RealTimeRayTracing -NoScene -Background "13,18,31"
powershell -ExecutionPolicy Bypass -File scripts\verify-scene.ps1 -Preset Vulkan-Release -BuildTarget RealTimeRayTracing -NoScene -Background "13,18,31"
```

**Acceptance (all four):**

| Check | Value |
|---|---|
| `RESULT` | `PASS` |
| `VALIDATION` | `no errors in stdout/stderr` |
| `content` | `>= 90%` — the quad covers the whole viewport, so almost every pixel differs from the clear colour |
| `centerMean` | `r` clearly above `b` (the red sphere is in the centre of the image) |
| PNG | red sphere, grey ground with a soft shadow, blue sky gradient. **No** solid red/green debug colours (those were the old compute shader's hit/miss output). |

**If `content` is `0%`:** the quad is not drawn. Check `BindShader` is called before `DrawQuad`, and that `Fullscreen.vert` reads locations 0 and 1 (the layout is `{ Vec3, Vec2 }`).

**If the image is upside-down:** `vTexCoord` was used with a `1.0 - vTexCoord.y` somewhere, or the ray was built from `gl_FragCoord`. Remove both; the construction in `RayTracing.frag` is correct by design (see the comment in `main`).

**If the whole frame is sky blue with no sphere:** the camera is looking the wrong way. `pitch = -19.0f`, `yaw = -56.4f` from `(-10, 7, 16)` must point at the origin — verify by computing `mCameraFront` (should be ≈ `(0.52, -0.33, -0.79)`).

**Commit:** `chore(rtt): verify RealTimeRayTracing on OpenGL and Vulkan, Debug and Release`

---

### P3-07 — Regression check

```powershell
powershell -ExecutionPolicy Bypass -File scripts\verify-scene.ps1 -Preset Vulkan-Debug -Scene 4
powershell -ExecutionPolicy Bypass -File scripts\verify-scene.ps1 -Preset Vulkan-Debug -BuildTarget WaveSimulation -NoScene -Background "255,255,255"
powershell -ExecutionPolicy Bypass -File scripts\verify-scene.ps1 -Preset Vulkan-Debug -BuildTarget AnimationTechniques -Scene 1 -Background "51,51,51"
```

Expected: three `RESULT : PASS`. This also proves P3-00 (`DrawQuad`) did not break anything.

**Commit:** none.
