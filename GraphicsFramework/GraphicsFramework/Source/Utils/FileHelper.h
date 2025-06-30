#pragma once

#include "Core/Core.h"

class FileHelper
{
public:
	static std::vector<char> ReadFile(std::string filePath)
	{
		std::ifstream file(filePath, std::ios::ate);

		if (!file.is_open())
		{
			std::cout << "Failed to open File: " << filePath << std::endl;
		}

		size_t fileSize = (size_t)file.tellg();
		std::vector<char> FileData(fileSize);

		file.seekg(0);
		file.read(FileData.data(), fileSize);

		file.close();

		return FileData;
	}

	static std::vector<char> ReadBinaryFile(std::string filePath)
	{
		std::ifstream file(filePath, std::ios::binary | std::ios::ate);

		if (!file.is_open())
		{
			std::cout << "Failed to open File: " << filePath << std::endl;
		}

		size_t fileSize = (size_t)file.tellg();
		std::vector<char> BinaryData(fileSize);

		file.seekg(0);
		file.read(BinaryData.data(), fileSize);

		file.close();

		return BinaryData;
	}

	static std::vector<std::string> SplitString(const std::string& sourceString, char splitbyChar)
	{
		std::stringstream sourceStringStream(sourceString);
		std::vector<std::string> Lines;
		std::string line;
		while (std::getline(sourceStringStream, line, splitbyChar))
		{
			Lines.push_back(line);
		}

		return Lines;
	}
};
