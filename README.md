# GraphicsFramework

GraphicsFramework is a C++17 rendering framework with OpenGL and Vulkan backends. The repository also contains sample applications, graphics tests, and a separate irradiance-map utility.

## Requirements

- Windows x64 and Microsoft Visual C++ (MSVC)
- CMake 3.22 or newer
- Ninja 1.10 or newer, including the `ninja` executable on `PATH`
- MSVC build tools
- Git submodules initialized under `GraphicsFramework/GraphicsFramework/ThirdParty`
- The Vulkan SDK when building the Vulkan renderer (its Vulkan loader is provided by the SDK)

The source dependencies are pinned as Git submodules. After cloning without `--recurse-submodules`, initialize them from the repository root:

```powershell
git submodule update --init --recursive
```

## Configure with Ninja Multi-Config

From the repository root, run:

```powershell
cmake --preset Ninja-Multi
```

This configures a multi-configuration Ninja build in `out/build/ninja-multi`. Select the renderer and build mode with one of the build presets:

- `OpenGL-Debug` (`Debug_OpenGL`)
- `OpenGL-Release` (`Release_OpenGL`)
- `Vulkan-Debug` (`Debug_Vulkan`)
- `Vulkan-Release` (`Release_Vulkan`)

The build directory and generated files are local artifacts and are not checked into the repository.

## Build with CMake

Build the framework library in a selected configuration:

```powershell
cmake --build --preset OpenGL-Debug --target GraphicsFramework --parallel
cmake --build --preset Vulkan-Debug --target GraphicsFramework --parallel
```

Use the `OpenGL-Release` or `Vulkan-Release` build preset for Release builds. The renderer-specific source files, compile definitions, and libraries are selected automatically from the configuration name.

The sample applications and graphics tests are optional because some legacy sample sources still use framework APIs that have since changed. To include their targets, configure with `cmake --preset Ninja-Multi -DGF_BUILD_SAMPLES=ON`. Update any affected sample source to the current framework API before building those targets.

## Projects

| Project | Description |
| --- | --- |
| [GraphicsFramework library](GraphicsFramework/GraphicsFramework/README.md) | Core engine and rendering library with OpenGL and Vulkan backends. |
| [AdvancedRenderingTechniques](GraphicsFramework/AdvancedRenderingTechniques/README.md) | Selectable rendering demonstrations, including motion blur. |
| [AnimationTechniques](GraphicsFramework/AnimationTechniques/README.md) | Animation and mechanics scenes. |
| [FlockingParticles](GraphicsFramework/FlockingParticles/README.md) | Particle flocking simulation. |
| [RealTimeRayTracing](GraphicsFramework/RealTimeRayTracing/README.md) | Real-time ray-tracing demonstration. |
| [UnitTests](GraphicsFramework/UnitTests/README.md) | Interactive graphics and framework tests. |
| [VulkanTest](GraphicsFramework/VulkanTest/README.md) | Standalone SDL/Vulkan sample, not currently added as a CMake target. |
| [WaveSimulation](GraphicsFramework/WaveSimulation/README.md) | Wave simulation demonstration. |
| [IrradianceMapApp](IrradianceMapApp/README.md) | Separate HDR irradiance-map utility. |
