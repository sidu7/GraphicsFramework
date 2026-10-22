# GraphicsFramework library

The core C++ rendering library and engine used by the repository's graphics applications. It contains the engine, scene and object systems, resource handling, and renderer interfaces, with backend implementations for OpenGL and Vulkan.

The library is built as the `GraphicsFramework` static-library target. Renderer selection is controlled by the Visual Studio solution configuration: `Debug_OpenGL`, `Release_OpenGL`, `Debug_Vulkan`, or `Release_Vulkan`.

See the [repository README](../../README.md) for prerequisites, solution generation, and build commands. The project uses the bundled dependencies under `ThirdParty` and currently requires 64-bit Windows and MSVC.
