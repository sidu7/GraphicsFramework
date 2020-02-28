#include "Transform.h"
#include "../../utils/JSONHelper.h"
#include "../Engine.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "../opengl/Shader.h"
#include "../opengl/Renderer.h"

void Transform::Update()
{
	mTranslationMatrix = glm::translate(glm::mat4(1.0f), mPosition);
	mRotationMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(mRotation.z), glm::vec3(0.0, 0.0, 1.0));
	mRotationMatrix = glm::rotate(mRotationMatrix, glm::radians(mRotation.y), glm::vec3(0.0, 1.0, 0.0));
	mRotationMatrix = glm::rotate(mRotationMatrix, glm::radians(mRotation.x), glm::vec3(1.0, 0.0, 0.0));
	mScaleMatrix = glm::scale(glm::mat4(1.0f), mScale);
	if (!Engine::Instance().stopMoving)
	{
		mPrevModelTransformation = mModelTransformation;
	}

	Renderer::Instance().GetRenderData().mTransform = this;
	SetModelTransformation();	
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
