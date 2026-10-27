# Scene baselines for `UnitTests`

Measured on 2026-10-27 with `scripts/verify-scene.ps1`, window 1296x759 (1280x720 content +
chrome), `Debug_OpenGL` and `Debug_Vulkan`, 1500 ms settle time.

`content` = percentage of pixels (right of the ImGui panels, below the title bar) that differ from
the detected background colour. `colored` = subset with a large channel spread.
`centerMean` = mean RGB of the middle 50 % of the window. `bg` is auto-detected as the most common
colour of the frame; for all five `UnitTests` scenes it is the black clear colour, reported as
`(2,2,2)` because of 6-bit quantisation.

| Scene | Backend | content | bgCov | colored | centerMean | bbox |
|---|---|---|---|---|---|---|
| 0 `ShaderTest` | Vulkan | 16.50 % | 83.5 % | 15.24 % | (40,39,39) | (328,212,640,360) |
| 0 `ShaderTest` | OpenGL | 16.51 % | 83.4 % | 15.27 % | (40,40,40) | (328,211,640,361) |
| 1 `VertexBufferTest` | Vulkan | 16.51 % | 83.5 % | 16.41 % | (30,57,78) | (328,212,640,360) |
| 1 `VertexBufferTest` | OpenGL | 16.51 % | 83.4 % | 11.84 % | (79,56,50) | (328,211,640,361) |
| 2 `IndexBufferTest` | Vulkan | 32.92 % | 67.1 % | 30.46 % | (119,79,118) | (328,211,640,361) |
| 2 `IndexBufferTest` | OpenGL | 32.92 % | 67.1 % | 30.46 % | (119,79,119) | (328,211,640,361) |
| 3 `UniformBufferTest` | Vulkan | 5.38 % | 94.6 % | 4.95 % | (21,13,18) | (471,268,354,212) |
| 3 `UniformBufferTest` | OpenGL | 5.41 % | 94.6 % | 4.98 % | (21,13,18) | (470,303,355,212) |
| 4 `TextureTest` | Vulkan | 1.71 % | 96.5 % | 0.49 % | (3,2,3) | (480,279,347,198) |
| 4 `TextureTest` | OpenGL | 3.39 % | 94.9 % | 1.63 % | (6,4,7) | (474,303,354,210) |

Scene 1 is animated and scenes 3/4 rotate, so `colored`, `centerMean` and `bbox` differ between
captures; `content` is the stable value to compare.

## Per-scene description

**0 `ShaderTest`** — full-window-height gradient triangle (red top, blue bottom-left, green
bottom-right on Vulkan; mirrored vertically). Fills ~16 % of the checked region.

**1 `VertexBufferTest`** — same triangle, animated, colours change over time, so `centerMean`
varies. Also ~16 %.

**2 `IndexBufferTest`** — large indexed quad, four-corner gradient (red/green/blue/white). The
biggest scene at ~33 %.

**3 `UniformBufferTest`** — a smaller quad (about 355x212 px) rotated about Z by the UBO matrices,
gradient coloured. `content≈5 %` is correct. If the UBO is not bound, this scene renders
**nothing** (`content=0 %`, `bbox=none`) while the ImGui panels still appear — that is the
classic "descriptors not written" signature.

**4 `TextureTest`** — the `res/Texture.jpg` night-city image on a quad of ~350x200 px, rotating
about Z, multiplied by the vertex colours. `content≈2 %` and `centerMean≈(3,2,3)` are correct:
the image itself is dark and is sampled through an sRGB format while the back buffer is UNORM,
so it renders darker than the JPEG on **both** backends. `colored` is under 1 % and must not be
used as the pass criterion for this scene.

## Tolerances

* Scenes 3 and 4 rotate: `content` varies with the captured angle. Observed range for scene 4 is
  1.7–3.5 %; scene 3 is 4–6 %.
* Compare like with like: same backend twice should agree within a few percent; Vulkan vs OpenGL
  should agree within ~1.5x for scenes 0–3.
* Failure thresholds that are safe: `content=0 %`, `bbox=none`, or a tenfold drop. The script's
  default threshold is 0.5 %, deliberately below the darkest real scene (`TextureTest`, ~1.6 %).
* Window size changes scale `bbox` linearly and leave the percentages roughly unchanged, so only
  percentages are comparable across machines.

## Other apps: known clear colours

The statistics compare against the clear colour, which is detected automatically. When you pass
`-Background` explicitly, use the value from the app's source:

| App | Clear colour in source | `-Background` |
|---|---|---|
| `UnitTests` (all scenes) | black | `0,0,0` |
| `RealTimeRayTracing` | `glClearColor(0,0,0,1)` | `0,0,0` |
| `WaveSimulation` | `SetClearColor(1,1,1)` | `255,255,255` |
| `FlockingParticles` | `SetClearColor(0.461305,0.627375,0.627375)` | `118,160,160` |

Note: while the Vulkan SDK is installed, the sample apps do **not** compile — their GLSL is
OpenGL-style (`uniform` outside a block, no `layout(location=…)`) but `add_framework_spirv()`
compiles it for Vulkan in every configuration. Only `UnitTests` is currently runnable, so the
sample rows above document intent rather than a verified run.

## Self-test of the statistics code

`-AnalyzeOnly` makes the pixel logic testable without an app. These are the expected results for
synthetic images (800x600, square at 400,300,200x200):

| Input | Command | Expected |
|---|---|---|
| white + red square | default | `content=14.88% bg=(254,254,254)(auto) bgCov=85.1% bbox=(400,300,200,200)` → PASS |
| all white | default | `content=0% bg=(254,254,254)(auto) bgCov=100% bbox=none` → FAIL |
| all black | default | `content=0% bg=(2,2,2)(auto) bgCov=100% bbox=none` → FAIL |
| white + red square | `-Background "0,0,0"` | `content=100% bg=(0,0,0)(manual) bbox=(320,40,480,560)` → PASS |
| white + red square | `-Background "255,255,255"` | `content=14.88% bg=(255,255,255)(manual) bbox=(400,300,200,200)` → PASS |

If a white background ever reports `bg=(2,2,2)` again, the 6-bit packing lost its `[int]` cast
(PowerShell bitwise operators keep the left operand's type, so `[byte]63 -shl 12` is `0`).

## Historical failures these numbers detect

| Symptom | Cause found on 2026-10-27 | Fixed in |
|---|---|---|
| Scene 3 `content=0 %`, process alive | `ShaderVK::GenerateShaderBindings()` used the descriptor set layout before `DescriptorSetVK::Init()` created it → `vkCreatePipelineLayout` got `VK_NULL_HANDLE`; no descriptor writes at draw time | `RHI/Vulkan/ShaderVK.cpp`, `DeviceVK.cpp`, `Internal/DescriptorSetVK.*` |
| Scene 4 exit `0xC0000094` during texture creation | 3-channel JPEG uploaded as `VK_FORMAT_R8G8B8_SRGB`, rejected for optimal-tiling sampled images; no `vkCmdCopyBufferToImage` / layout transition | `RHI/Vulkan/TextureVK.cpp` |
