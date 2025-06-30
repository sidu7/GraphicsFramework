#include "ShaderManager.h"

#include "RenderingFactory.h"
#include "Utils/FileHelper.h"
#include "Utils/JSONHelper.h"

DISABLE_OPTIMIZATION
void ShaderManager::Init(std::string ShadersListFilePath)
{
	rapidjson::Document ShadersList = JSONHelper::ParseFile(ShadersListFilePath);

	rapidjson::Value::Object Root = ShadersList.GetObject();
	if (Root.HasMember("Shaders"))
	{
		rapidjson::Value::Array ShadersArray = Root["Shaders"].GetArray();

		for (int32_t i = 0; i < ShadersArray.Size(); ++i)
		{
			rapidjson::Value::Object Shader = ShadersArray[i].GetObject();
			if (Shader.HasMember("Api"))
			{
				if (RenderingFactory::Instance()->GetApiName() != Shader["Api"].GetString())
				{
					continue;
				}
			}

			if (Shader.HasMember("Id") && Shader.HasMember("Path"))
			{
				mShadersList.emplace(Shader["Id"].GetString(), Shader["Path"].GetString());
			}
		}
	}
}

std::vector<char> ShaderManager::GetShaderContents(const std::string& shaderId)
{
	auto foundItr = mShadersList.find(shaderId);
	if (foundItr != mShadersList.end())
	{
		const std::string& filePath = foundItr->second;

#if defined(RENDERER_OPENGL)
		return FileHelper::ReadFile(filePath);
#elif defined(RENDERER_VULKAN)
		return FileHelper::ReadBinaryFile(filePath + ".spv");
#endif
	}

	return std::vector<char>();
}
ENABLE_OPTIMIZATION