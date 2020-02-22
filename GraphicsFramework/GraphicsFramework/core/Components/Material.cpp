#include "Material.h"
#include "../opengl/Shader.h"
#include "../opengl/Texture.h"
#include "../../utils/JSONHelper.h"

void Material::Update(Shader* shader)
{
}

void Material::Serialize(rapidjson::Value::Object data)
{
	if (data.HasMember("Diffuse"))
	{
		mDiffuse = JSONHelper::GetVec3F(data["Diffuse"].GetArray());
		pTexture = nullptr;
	}
	else
	{
		pTexture = new Texture(data["Texture"].GetString());
		mDiffuse = glm::vec3(0.0f);
	}
	mSpecular = JSONHelper::GetVec3F(data["Specular"].GetArray());
	mShininess = data["Shininess"].GetFloat();
	if(data.HasMember("WireMesh"))
	{
		mWireMesh = data["WireMesh"].GetBool();
	}
	if (data.HasMember("DebugMesh"))
	{
		mDebugMesh = data["DebugMesh"].GetBool();
	}
}
