#include "Move.h"
#include "../Engine.h"
#include "../Time.h"
#include "../Object.h"
#include "Transform.h"

void Move::Update(Shader* shader)
{
	if (Engine::Instance().stopMoving)
	{
		return;
	}
	mWaitTime += Time::Instance().deltaTime;
	if (mWaiting && mWaitTime < mWait)
	{
		return;
	}
	float deltaTime = Time::Instance().deltaTime;
	mCurrentTime += deltaTime;
	if (mHorizontal)
	{
		if (mFlip)
		{
			mOwner->GetComponent<Transform>()->mPosition.x += mSpeed * deltaTime;
		}
		else
		{
			mOwner->GetComponent<Transform>()->mPosition.x -= mSpeed * deltaTime;
		}
	}
	else
	{
		if (mFlip)
		{
			mOwner->GetComponent<Transform>()->mPosition.y += mSpeed * deltaTime;
		}
		else
		{
			mOwner->GetComponent<Transform>()->mPosition.y -= mSpeed * deltaTime;
		}
	}
	if (mCurrentTime >= mTime)
	{
		mFlip = !mFlip;
		mCurrentTime = 0.0f;
		mWaitTime = 0.0f;
		mWaiting = true;
	}
}

void Move::Serialize(rapidjson::Value::Object data)
{
	mTime = data["Time"].GetFloat();
	mSpeed = data["Speed"].GetFloat();
	mHorizontal = data["Horizontal"].GetBool();
	mCurrentTime = 0.0f;
	mFlip = data["Flip"].GetBool();
	mWait = data["Wait"].GetFloat();
	mWaitTime = 0.0f;
}
