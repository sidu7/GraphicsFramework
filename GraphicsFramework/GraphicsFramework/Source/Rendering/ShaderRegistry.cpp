/* Start Header -------------------------------------------------------
Copyright (C) 2018-2019 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the prior
written consent of DigiPen Institute of Technology is prohibited.
Author: Sidhant Tumma
- End Header --------------------------------------------------------*/

#include "ShaderRegistry.h"

#include <algorithm>

void ShaderRegistry::Init(const std::string& shaderBasePath, const RegisterShadersFn& registerShaders)
{
    mShaderBasePath = shaderBasePath;

    // Ensure paths end with '/'
    if (!mShaderBasePath.empty() && mShaderBasePath.back() != '/')
        mShaderBasePath += '/';

    // Let application register its shaders
    if (registerShaders)
        registerShaders(this);
}

void ShaderRegistry::RegisterShader(int32_t id,
    const std::string& glslVertex,
    const std::string& glslFragment,
    const std::string& spirvVertex,
    const std::string& spirvFragment)
{
    ShaderEntry entry;
    entry.glslVertex = mShaderBasePath + glslVertex;
    entry.glslFragment = mShaderBasePath + glslFragment;
    entry.spirvVertex = mShaderBasePath + spirvVertex;
    entry.spirvFragment = mShaderBasePath + spirvFragment;

    mShaders[id] = entry;
}

const std::string& ShaderRegistry::GetGLSLPath(int32_t id, ShaderStage stage) const
{
    static const std::string empty;
    auto it = mShaders.find(id);
    if (it == mShaders.end()) return empty;

    switch (stage)
    {
        case ShaderStage::Vertex:   return it->second.glslVertex;
        case ShaderStage::Fragment: return it->second.glslFragment;
    }
    return empty;
}

const std::string& ShaderRegistry::GetSPIRVPath(int32_t id, ShaderStage stage) const
{
    static const std::string empty;
    auto it = mShaders.find(id);
    if (it == mShaders.end()) return empty;

    switch (stage)
    {
        case ShaderStage::Vertex:   return it->second.spirvVertex;
        case ShaderStage::Fragment: return it->second.spirvFragment;
    }
    return empty;
}

const std::string& ShaderRegistry::GetGLSLComputePath(int32_t id) const
{
    return GetGLSLPath(id, ShaderStage::Vertex);
}

const std::string& ShaderRegistry::GetSPIRVComputePath(int32_t id) const
{
    return GetSPIRVPath(id, ShaderStage::Vertex);
}
