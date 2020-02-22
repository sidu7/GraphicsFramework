#include "Flock.h"

#include "core/Object.h"
#include "core/Time.h"
#include "core/Components/Transform.h"
#include "opengl/Shader.h"
#include "utils/MathHelper.h"

Flock Flock::instance;

void Flock::Update(Shader* shader)
{
	glm::vec3 X(1.0f, 0.0f, 0.0f);
	glm::vec3 Y(0.0f, 1.0f, 0.0f);
	glm::vec3 Z(0.0f, 0.0f, 1.0f);
	const float PI = 22.0f / 7.0f;
	Transform* trans = mOwner->GetComponent<Transform>();
	mVelocity += mAcceleration * Time::Instance().deltaTime;
	trans->mPosition += mVelocity * Time::Instance().deltaTime;
	//trans->mRotation.x = glm::acos(glm::dot(mVelocity, X)) * 180.0f / PI;
	//trans->mRotation.y = glm::acos(glm::dot(mVelocity, Y)) * 180.0f / PI;
	//trans->mRotation.z = glm::acos(glm::dot(mVelocity, Z)) * 180.0f / PI;
	//trans->mModelTransformation = MathHelper::RotationFromDirection(glm::normalize(mVelocity)) * trans->mModelTransformation;
	//shader->SetUniformMat4f("model", trans->mModelTransformation);
}

void Flock::Serialize(rapidjson::Value::Object data)
{
}
