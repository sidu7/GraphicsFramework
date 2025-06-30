#pragma once

#include "Core/Core.h"
#include "Rendering/Shader.h"

class ShaderManager : public Singleton<ShaderManager>
{
public:

	void Init(std::string ShadersListFilePath);

	const ShaderSource& GetShaderSource(const std::string& shaderId);

protected:
	std::vector<char> GetShaderContents(rapidjson::Value& shaderInfo);

	std::unordered_map<std::string, ShaderSource> mShadersList;
};