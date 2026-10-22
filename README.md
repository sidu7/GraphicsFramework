# GraphicsFramework

GraphicsFramework is a C++17 rendering framework with OpenGL and Vulkan backends. The repository also contains sample applications, graphics tests, and a separate irradiance-map utility.

## Requirements

- Windows x64 and Microsoft Visual C++ (MSVC)
- CMake 4.2 or newer for the Visual Studio 2026 generator (the project itself requires CMake 3.22 or newer)
- Visual Studio 2026 (or another CMake-supported Visual Studio generator)
- Git submodules initialized under `GraphicsFramework/GraphicsFramework/ThirdParty`
- The Vulkan SDK when building the Vulkan renderer (its Vulkan loader is provided by the SDK)

The source dependencies are pinned as Git submodules. After cloning without `--recurse-submodules`, initialize them from the repository root:

```powershell
git submodule update --init --recursive
```

## Generate a Visual Studio solution

From the repository root, run:

```powershell
cmake -S . -B build-vs -G "Visual Studio 18 2026" -A x64
```

With the Visual Studio 2026 generator, CMake generates `build-vs/GraphicsFramework.slnx`. Open that solution in Visual Studio. The solution configuration selector provides:

- `Debug_OpenGL`
- `Release_OpenGL`
- `Debug_Vulkan`
- `Release_Vulkan`

Select a configuration to choose both the build mode and renderer. The generated solution and build output are local build artifacts and are not checked into the repository.

To generate a traditional `.sln` with a Visual Studio 2022 generator, when that generator is installed, use:

```powershell
cmake -S . -B build-vs2022 -G "Visual Studio 17 2022" -A x64
```

## Build with CMake

Build the framework library in a selected configuration:

```powershell
cmake --build build-vs --config Debug_OpenGL --target GraphicsFramework --parallel
cmake --build build-vs --config Debug_Vulkan --target GraphicsFramework --parallel
```

Use `Release_OpenGL` or `Release_Vulkan` in place of the Debug configuration for Release builds. The renderer-specific source files, compile definitions, and libraries are selected automatically from the configuration name.

The sample applications and graphics tests are optional because some legacy sample sources still use framework APIs that have since changed. To include their targets when generating the solution, add `-DGF_BUILD_SAMPLES=ON` to the configure command. Update any affected sample source to the current framework API before building those targets.

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
