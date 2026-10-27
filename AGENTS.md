# AGENTS.md — GraphicsFramework

## Prerequisites & Platform
- **Windows x64 + MSVC only.** CMake validates this at configure time (`GF_PROJECT_DIR/CMakeLists.txt:34`). Linux/WSL/GCC will fail.
- Requires CMake 3.22+, Ninja 1.10+ (on `PATH`), and the Vulkan SDK (when building Vulkan configs).

## Submodule gotcha
Submodules live at the deeply nested path `GraphicsFramework/GraphicsFramework/ThirdParty/*` (the repo root contains a `GraphicsFramework/` subdirectory which itself is named `GraphicsFramework`). Initialize from the **repo root**:
```powershell
git submodule update --init --recursive
```

## Build flow (Ninja Multi-Config)

1. **Configure** (once, from repo root):
   ```powershell
   cmake --preset Ninja-Multi
   ```
   Binary output goes to `build/ninja-multi/`.

2. **Build targets** (explicit — all sample/test targets are `EXCLUDE_FROM_ALL`):
   ```powershell
   cmake --build --preset OpenGL-Debug --target GraphicsFramework --parallel
   cmake --build --preset Vulkan-Debug --target UnitTests --parallel
   ```
   Replace `OpenGL-Debug` with `OpenGL-Release`, `Vulkan-Debug`, or `Vulkan-Release`.

3. **Available presets**:
   | Preset | CMake config | Renderer |
   |---|---|---|
   | `OpenGL-Debug` | `Debug_OpenGL` | OpenGL |
   | `OpenGL-Release` | `Release_OpenGL` | OpenGL |
   | `Vulkan-Debug` | `Debug_Vulkan` | Vulkan |
   | `Vulkan-Release` | `Release_Vulkan` | Vulkan |

## Key quirks

- **Renderer selection**: Source files in `Source/Rendering/OpenGL/` and `Source/Rendering/Vulkan/` are conditionally compiled based on config name (`RENDERER_OPENGL` / `RENDERER_VULKAN` compile definitions, see `Source/CMakeLists.txt`).
- **Samples are opt-in**: Build with `-DGF_BUILD_SAMPLES=OFF` to exclude all sample apps and UnitTests.
- **Sample targets are not in the default build**: Due to `EXCLUDE_FROM_ALL`, building `GraphicsFramework` does **not** build samples. You must explicitly request each target.
- **Runtime DLLs**: Sample apps copy DLLs from `GraphicsFramework/DLLs/` at post-build time. If DLLs are missing, apps may fail to run.
- **IrradianceMapApp** (`IrradianceMapApp/`): Standalone CLI utility — **not** a CMake target. Built manually or with MSVC project files.
- **VulkanTest** (`GraphicsFramework/VulkanTest/`): Legacy standalone sample — **not** a CMake target.
- **`.spv` files** are gitignored (generated at build time).
- **`CMakeUserPresets.json`** is gitignored; prefer `CMakePresets.json` as the source of truth.

## Directory map

| Path | Contents |
|---|---|
| `GraphicsFramework/GraphicsFramework/Source/` | Core library — `Core/`, `Rendering/`, `Utils/` |
| `GraphicsFramework/GraphicsFramework/ThirdParty/` | Submodule dependencies (assimp, glm, SDL, GLEW, Imgui, Vulkan SDK headers, volk, vma, SPIRV-Reflect, rapidjson, stb) |
| `GraphicsFramework/{FlockingParticles,AdvancedRenderingTechniques,AnimationTechniques,RealTimeRayTracing,WaveSimulation,UnitTests}/` | Sample apps & tests |
| `GraphicsFramework/VulkanTest/` | Legacy standalone sample (not CMake) |
| `GraphicsFramework/DLLs/` | Runtime DLLs copied to sample output dirs |
| `IrradianceMapApp/` | Standalone HDR irradiance-map CLI tool |
| `scripts/` | Developer tooling — `verify-scene.ps1` builds, runs and screenshots one scene |

## Testing
- `UnitTests` target runs interactive graphics tests (shaders, buffers, textures) through the framework.
- Run with: `cmake --build --preset <preset> --target UnitTests --parallel`, then execute the built binary.

## Visual verification (does it actually draw?)
- Rendering is only verifiable from a screenshot: the apps are GUI programs, and normal
  screenshots (`CopyFromScreen`) return black for both backends.
- Use `scripts/verify-scene.ps1`; it builds, runs one scene, captures the SDL window with
  `PrintWindow(PW_RENDERFULLCONTENT)`, saves the PNG plus redirected stdout/stderr (Vulkan
  validation messages) and prints pixel statistics:
  ```powershell
  powershell -ExecutionPolicy Bypass -File scripts\verify-scene.ps1 -Preset Vulkan-Debug -Scene 4 -Build
  ```
  Exit code `0` = rendered, `2` = crashed or nothing drawn. Scenes: `0` Shader, `1` VertexBuffer,
  `2` IndexBuffer, `3` UniformBuffer, `4` Texture.
- The capture is OS-level, so it works for any app built with `add_framework_app` and any RHI
  (Vulkan, OpenGL, …); only the `-scene` numbers are UnitTests-specific. Use `-BuildTarget <name>`
  and `-NoScene` for a sample, `-Background "r,g,b"` for a non-black clear colour, and
  `-AnalyzeOnly <png>` to analyse an existing screenshot.
- Full procedure, baseline numbers, failure signatures and manual capture: skill
  `.opencode/skills/visual-verify/SKILL.md` (load with `@visual-verify`).
