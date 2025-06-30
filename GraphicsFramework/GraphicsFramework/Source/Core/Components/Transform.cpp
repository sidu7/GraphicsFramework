#include "Transform.h"
#include "Utils/JSONHelper.h"
#include "Core/Engine.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "Rendering/Shader.h"
#include "Rendering/Renderer.h"
#include "Rendering/RenderingFactory.h"

Transform::Transform() :
	mPosition(0.f),
	mScale(0.f),
	mRotation(0.f),
	mFront(0.f),
	mTranslationMatrix(1.f),
	mRotationMatrix(1.f),
	mScaleMatrix(1.f),
	mModelTransformation(1.f),
	mPrevModelTransformation(1.f),
	mMatricesUBO(nullptr)
{
	mMatricesUBO = RenderingFactory::Instance()->CreateUniformBuffer();
	mMatricesUBO->Init(sizeof(ObjectMatricesUBO), Engine::Instance()->GetSettings().mTransformBindingPoint);
}

Transform::~Transform()
{
	delete mMatricesUBO;
}

void Transform::Update()
{
	mTranslationMatrix = glm::translate(glm::mat4(1.0f), mPosition);
	mRotationMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(mRotation.z), glm::vec3(0.0, 0.0, 1.0));
	mRotationMatrix = glm::rotate(mRotationMatrix, glm::radians(mRotation.y), glm::vec3(0.0, 1.0, 0.0));
	mRotationMatrix = glm::rotate(mRotationMatrix, glm::radians(mRotation.x), glm::vec3(1.0, 0.0, 0.0));
	mScaleMatrix = glm::scale(glm::mat4(1.0f), mScale);
	if (!Engine::Instance()->stopMoving)
	{
		mPrevModelTransformation = mModelTransformation;
	}

	SetModelTransformation();

	if (mMatricesUBO)
	{
		ObjectMatricesUBO UboData{ glm::inverse(mModelTransformation), mModelTransformation, mPrevModelTransformation };
		mMatricesUBO->AddData(sizeof(UboData), &UboData);
	}
}

void Transform::Serialize(rapidjson::Value::Object data)
{
	mPosition = JSONHelper::GetVec3F(data["Position"].GetArray());
	mScale = JSONHelper::GetVec3F(data["Scale"].GetArray());
	mRotation = JSONHelper::GetVec3F(data["Rotation"].GetArray());
	mModelTransformation = glm::mat4(1.0f);
	if(data.HasMember("Front"))
	{
		mFront = JSONHelper::GetVec3F(data["Front"].GetArray());
	}
}

void Transform::SetModelTransformation()
{
	mModelTransformation = mTranslationMatrix * mRotationMatrix * mScaleMatrix;	
}
