#include "Material.h"
#include "OpenGL/Shader.h"
#include "OpenGL/Texture.h"
#include "Utils/JSONHelper.h"

void Material::Update()
{
	Renderer::Instance().GetRenderData().mMaterial = this;
}

void Material::Serialize(rapidjson::Value::Object data)
{
	if (data.HasMember("Diffuse"))
	{
		mDiffuse = JSONHelper::GetVec3F(data["Diffuse"].GetArray());
		pTexture = nullptr;
	}
	if(data.HasMember("Texture"))
	{
		pTexture = new Texture(data["Texture"].GetString());
		mDiffuse = glm::vec3(0.0f);
	}
	if (data.HasMember("Specular"))
	{
		mSpecular = JSONHelper::GetVec3F(data["Specular"].GetArray());
	}
	if (data.HasMember("Shininess"))
	{
		mShininess = data["Shininess"].GetFloat();
	}
	if(data.HasMember("WireMesh"))
	{
		mWireMesh = data["WireMesh"].GetBool();
	}
	if (data.HasMember("DebugMesh"))
	{
		mDebugMesh = data["DebugMesh"].GetBool();
	}
	if (data.HasMember("Lighting"))
	{
		mLighting = data["Lighting"].GetBool();
	}
	if(data.HasMember("Mesh"))
	{
		mMesh = data["Mesh"].GetBool();
	}
	if(data.HasMember("DebugColor"))
	{
		mDebugColor = JSONHelper::GetVec3F(data["DebugColor"].GetArray());
	}
}
