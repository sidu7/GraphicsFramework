#include "Flock.h"

#include "core/Object.h"
#include "core/Time.h"
#include "core/Components/Transform.h"
#include "opengl/Shader.h"
#include "utils/MathHelper.h"

Flock Flock::instance;

void Flock::Update(Shader* shader)
{
	Transform* trans = mOwner->GetComponent<Transform>();
	mVelocity += mAcceleration * Time::Instance().deltaTime;
	trans->mPosition += mVelocity * Time::Instance().deltaTime;
	//trans->mModelTransformation = MathHelper::RotationFromDirection(glm::normalize(mVelocity)) * trans->mModelTransformation;
	//shader->SetUniformMat4f("model", trans->mModelTransformation);
}

void Flock::Serialize(rapidjson::Value::Object data)
{
}
