#include "Material.h"
#include "../opengl/Shader.h"
#include "../opengl/Texture.h"
#include "../JSONHelper.h"

void Material::Update(Shader* shader)
{
	shader->SetUniform3f("diffuse", mDiffuse.x, mDiffuse.y, mDiffuse.z);
	shader->SetUniform3f("specular", mSpecular.x, mSpecular.y, mSpecular.z);
	shader->SetUniform1f("shininess", mShininess);
	if (pTexture)
	{
		pTexture->Bind(8);
		shader->SetUniform1i("texDiff", 8);
	}
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
}
