#include "ShaderManager.h"

#include "RenderingFactory.h"
#include "Utils/FileHelper.h"
#include "Utils/JSONHelper.h"

void ShaderManager::Init(std::string ShadersListFilePath)
{
	rapidjson::Document ShadersList = JSONHelper::ParseFile(ShadersListFilePath);

	rapidjson::Value::Object Root = ShadersList.GetObject();
	if (Root.HasMember("Shaders"))
	{
		rapidjson::Value::Array ShadersArray = Root["Shaders"].GetArray();

		for (int32_t i = 0; i < ShadersArray.Size(); ++i)
		{
			rapidjson::Value::Object shader = ShadersArray[i].GetObject();
			if (shader.HasMember("Id"))
			{
				ShaderSource shaderSource;
				if (shader.HasMember("VertexShader"))
				{
					shaderSource.vertexSource = GetShaderContents(shader["VertexShader"]);
				}
				if (shader.HasMember("FragmentShader"))
				{
					shaderSource.fragmentSource = GetShaderContents(shader["FragmentShader"]);
				}

				mShadersList.emplace(shader["Id"].GetString(), shaderSource);
			}
		}
	}
}

const ShaderSource& ShaderManager::GetShaderSource(const std::string& shaderId)
{
	auto FoundItr = mShadersList.find(shaderId);
	if (FoundItr != mShadersList.end())
	{
		return FoundItr->second;
	}

	return ShaderSource();
}

std::vector<char> ShaderManager::GetShaderContents(rapidjson::Value& shaderInfo)
{
	std::string shaderFilePath;

	// shader variants exists, get the appropriate shaderFilePath
	if (shaderInfo.IsArray())
	{
		rapidjson::Value::Array shaderVariantsInfo = shaderInfo.GetArray();
		for (int32_t i = 0; i < shaderVariantsInfo.Size(); ++i)
		{
			if (shaderVariantsInfo[i].IsObject())
			{
				rapidjson::Value::Object VariantInfo = shaderVariantsInfo[i].GetObject();
				if (VariantInfo.HasMember("Api") && VariantInfo.HasMember("Variant"))
				{
					if (RenderingFactory::Instance()->GetApiName() == VariantInfo["Api"].GetString())
					{
						shaderFilePath = VariantInfo["Variant"].GetString();
						break;
					}
				}
			}
		}
	}
	else if (shaderInfo.IsString())
	{
		shaderFilePath = shaderInfo.GetString();
	}
	else
	{
		std::cout << "Invalid entry in Shaders list file." << std::endl;
		return std::vector<char>();
	}

	if(!shaderFilePath.empty())
	{
#if defined(RENDERER_OPENGL)
		return FileHelper::ReadFile(shaderFilePath);
#elif defined(RENDERER_VULKAN)
		return FileHelper::ReadBinaryFile(shaderFilePath + ".spv");
#endif
	}

	return std::vector<char>();
}