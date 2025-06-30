#include "Material.h"
#include "Rendering/Shader.h"
#include "Rendering/Texture.h"
#include "Rendering/Renderer.h"
#include "Rendering/RenderingFactory.h"
#include "Utils/JSONHelper.h"
#include "Rendering/UniformBuffer.h"
#include "Shape.h"
#include "Core/Engine.h"

Material::Material() :
	mDiffuse(0.f),
	mDebugColor(0.f),
	mSpecular(0.f),
	mShininess(0.f),
	pTexture(nullptr),
	mLighting(true),
	mMaterialUBO(nullptr)
{
	mMaterialUBO = RenderingFactory::Instance()->CreateUniformBuffer();
	mMaterialUBO->Init(sizeof(MaterialDataUBO), Engine::Instance()->GetSettings().mMaterialBindingPoint);
}

Material::~Material()
{
	delete mMaterialUBO;
}

void Material::Update()
{
	glm::vec3 DiffuseColor = mDiffuse;
	if (mOwner)
	{
		Shape* shape = mOwner->GetComponent<Shape>();
		if (shape && (shape->mDebugMesh || shape->mWireMesh))
		{
			DiffuseColor = mDebugColor;
		}
	}

	if (mMaterialUBO)
	{
		MaterialDataUBO UboData{ glm::vec4(DiffuseColor, 1.0f), glm::vec4(mSpecular, 1.0f), mShininess, mLighting };
		mMaterialUBO->AddData(sizeof(UboData), &UboData);
	}
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
		pTexture = RenderingFactory::Instance()->CreateTexture();
		pTexture->Init(data["Texture"].GetString());
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
	if (data.HasMember("Lighting"))
	{
		mLighting = data["Lighting"].GetBool();
	}
	if(data.HasMember("DebugColor"))
	{
		mDebugColor = JSONHelper::GetVec3F(data["DebugColor"].GetArray());
	}
}
