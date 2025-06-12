#pragma once

#include "Core/Core.h"

class FileHelper
{
public:
	static std::string ReadFile(std::string filePath)
	{
		std::ifstream file(filePath);

		//TODO: Check whether shader file has actually opened
		std::stringstream fileContent;

		fileContent << file.rdbuf();

		file.close();

		return fileContent.str();
	}
};
