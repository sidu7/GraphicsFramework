#pragma once

#include "Core/Core.h"

class ShaderManager : public Singleton<ShaderManager>
{
public:

	void Init(std::string ShadersListFilePath);

	std::vector<char> GetShaderContents(const std::string& shaderId);

protected:
	std::unordered_map<std::string, std::string> mShadersList;
};