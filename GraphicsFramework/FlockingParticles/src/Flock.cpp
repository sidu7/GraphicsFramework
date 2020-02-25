#include "Flock.h"

#include "core/Object.h"
#include "core/Time.h"
#include "core/Components/Transform.h"
#include "opengl/Shader.h"
#include "utils/MathHelper.h"

#include <glm/gtx/quaternion.hpp>

Flock Flock::instance;

void Flock::Update(Shader* shader)
{
	const float PI = 22.0f / 7.0f;
	Transform* trans = mOwner->GetComponent<Transform>();
	mVelocity += mAcceleration * Time::Instance().deltaTime;
	trans->mPosition += mVelocity * Time::Instance().deltaTime;
	glm::vec3 Y(0.0f, 1.0f, 0.0f);
	glm::vec3 front = glm::normalize(mVelocity);	
	trans->mRotationMatrix = toMat4(glm::rotation(Y, front));
	trans->SetModelTransformation(shader);
}

void Flock::Serialize(rapidjson::Value::Object data)
{
}
