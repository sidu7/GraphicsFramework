#include "Flock.h"

#include "Core/Engine.h"
#include "Core/ShapeManager.h"
#include "Core/Object.h"
#include "Core/Time.h"
#include "Core/Components/Transform.h"
#include "Rendering/Shader.h"
#include "Rendering/Renderer.h"
#include "Utils/MathHelper.h"

#include <glm/gtx/quaternion.hpp>

Flock Flock::instance;

NeighboursData::NeighboursData() : mFlockHeading(glm::vec3(0.0f)), mFlockCenter(glm::vec3(0.0f)), mAvoidanceDirection(glm::vec3(0.0f))
{
}

void NeighboursData::Clear()
{
	mFlockHeading = glm::vec3(0.0f);
	mFlockCenter = glm::vec3(0.0f);
	mAvoidanceDirection = glm::vec3(0.0f);
}

void Flock::Update()
{
	const float PI = 22.0f / 7.0f;
	Transform* trans = mOwner->GetComponent<Transform>();
	glm::vec3 Y(0.0f, 1.0f, 0.0f);
	if(!Engine::Instance()->mPause)
	{
		mPreviousVelocity = mVelocity;
		mVelocity += mAcceleration * Time::Instance()->deltaTime;
		float speed = glm::length(mVelocity);
		glm::vec3 dir = mVelocity / speed;
		speed = glm::clamp(speed, mMinSpeed, mMaxSpeed);
		mVelocity = dir * speed;
		trans->mPosition += 0.5f * Time::Instance()->deltaTime * (mVelocity + mPreviousVelocity);
		mFront = dir;
		mNormal = glm::normalize(glm::cross(Y, mFront));
		mAcceleration = glm::vec3(0.0f);
	}

	/*glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, trans->mPosition);
	model *= MathHelper::RotationFromDirection(glm::normalize(mNormal));
	model = glm::scale(model, trans->mScale);
	auto data = ShapeManager::Instance()->mShapes[LINE];
	Renderer::Instance()->AddDebugRenderData(DebugData(data.first,data.second,model, GL_LINES,glm::vec3(1.0f,0.0f,0.0f)));*/
	
	glm::vec3 front = glm::normalize(mVelocity);	
	trans->mRotationMatrix = toMat4(glm::rotation(Y, front));
	trans->SetModelTransformation();
}

void Flock::Serialize(rapidjson::Value::Object data)
{
}
