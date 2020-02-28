#include "RotateInCircle.h"
#include "../Engine.h"
#include "../Time.h"
#include "../Object.h"
#include "Transform.h"

void RotateInCircle::Update()
{
	if (Engine::Instance().stopMoving)
	{
		return;
	}
	mAngle += Time::Instance().deltaTime * mRotationFactor;
	mTParam += Time::Instance().deltaTime;
	mOwner->GetComponent<Transform>()->mPosition.x = mRadius * glm::cos(mTParam * mSpeed);
	mOwner->GetComponent<Transform>()->mPosition.z = mRadius * glm::sin(mTParam * mSpeed);
	mOwner->GetComponent<Transform>()->mRotation.y = mAngle;
}

void RotateInCircle::Serialize(rapidjson::Value::Object data)
{
	mRadius = data["Radius"].GetFloat();
	mRotationFactor = data["RotationFactor"].GetFloat();
	mSpeed = data["Speed"].GetFloat();
	mTParam = 0.0f;
	mAngle = 0.0f;
}
