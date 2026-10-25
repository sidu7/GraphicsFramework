/* Start Header -------------------------------------------------------
Copyright (C) 2018-2019 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the prior
written consent of DigiPen Institute of Technology is prohibited.
Author: Sidhant Tumma
- End Header --------------------------------------------------------*/

#pragma once

#include "Core/Core.h"
#include <functional>

// ShaderRegistry maps application-defined int32_t shader IDs to file paths.
// Each application defines its own enum for shader IDs and registers shaders
// during init. The registry is completely agnostic to what shaders exist.

class ShaderRegistry : public Singleton<ShaderRegistry>
{
public:
    enum class ShaderStage : uint32_t
    {
        Vertex,
        Fragment
    };

    using RegisterShadersFn = std::function<void(ShaderRegistry*)>;

    void Init(const std::string& shaderBasePath, const RegisterShadersFn& registerShaders);

    // Register a shader with the registry. For graphics shaders, fragment path is non-empty.
    // For compute shaders, pass empty strings for the fragment paths.
    void RegisterShader(int32_t id,
        const std::string& glslVertex,
        const std::string& glslFragment,
        const std::string& spirvVertex,
        const std::string& spirvFragment);

    // Lookup GLSL source paths (for OpenGL runtime compilation)
    const std::string& GetGLSLPath(int32_t id, ShaderStage stage) const;

    // Lookup SPIR-V paths (for Vulkan pre-compiled shaders)
    const std::string& GetSPIRVPath(int32_t id, ShaderStage stage) const;

    // Compute shader lookup — uses the vertex slot (fragment is empty)
    const std::string& GetGLSLComputePath(int32_t id) const;
    const std::string& GetSPIRVComputePath(int32_t id) const;

private:
    struct ShaderEntry {
        std::string glslVertex;
        std::string glslFragment;
        std::string spirvVertex;
        std::string spirvFragment;
    };

    std::unordered_map<int32_t, ShaderEntry> mShaders;
    std::string mShaderBasePath;
};
