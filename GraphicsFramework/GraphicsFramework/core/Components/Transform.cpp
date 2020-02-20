#include "Transform.h"
#include "../JSONHelper.h"
#include "../Engine.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "../opengl/Shader.h"

void Transform::Update(Shader* shader)
{
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, mPosition);
	model = glm::rotate(model, glm::radians(mRotation.z), glm::vec3(0.0, 0.0, 1.0));
	model = glm::rotate(model, glm::radians(mRotation.y), glm::vec3(0.0, 1.0, 0.0));
	model = glm::rotate(model, glm::radians(mRotation.x), glm::vec3(1.0, 0.0, 0.0));
	model = glm::scale(model, mScale);
	if (!Engine::Instance().stopMoving)
	{
		mPrevModelTransformation = mModelTransformation;
	}
	mModelTransformation = model;
	shader->SetUniformMat4f("model", mModelTransformation);
	shader->SetUniformMat4f("prevmodel", mPrevModelTransformation);
	shader->SetUniformMat4f("normaltr", glm::inverse(mModelTransformation));
}

void Transform::Serialize(rapidjson::Value::Object data)
{
	mPosition = JSONHelper::GetVec3F(data["Position"].GetArray());
	mScale = JSONHelper::GetVec3F(data["Scale"].GetArray());
	mRotation = JSONHelper::GetVec3F(data["Rotation"].GetArray());
	mModelTransformation = glm::mat4(1.0f);
}