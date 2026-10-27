#include "Flock.h"
#include "Core/ComponentFactory.h"
#include "Core/Engine.h"
#include "Core/ShapeManager.h"
#include "Core/Object.h"
#include "Core/Time.h"
#include "Core/Components/Transform.h"
#include "Rendering/Shader.h"
#include "Rendering/Renderer.h"
#include "Utils/MathHelper.h"

#include <glm/gtx/quaternion.hpp>

void RegisterFlockComponent();

static const bool g_FlockRegistrarDummy = []() { RegisterFlockComponent(); return true; }();

static void RegisterFlockComponent()
{
    ComponentFactory::Instance()->Register("Flock",
        []() { return new Flock(); },
        std::type_index(typeid(Flock)));
}

NeighboursData::NeighboursData() : mFlockHeading(glm::vec3(0.0f)), mFlockCenter(glm::vec3(0.0f)), mAvoidanceDirection(glm::vec3(0.0f))
{
}

void NeighboursData::Clear()
{
	mFlockHeading = glm::vec3(0.0f);
	mFlockCenter = glm::vec3(0.0f);
	mAvoidanceDirection = glm::vec3(0.0f);
}

void Flock::Init(Object* owner)
{
	mOwner = owner;
	mNormal = glm::vec3(0.0f, 1.0f, 0.0f);
	mVelocity = glm::vec3(0.0f);
	mPreviousVelocity = glm::vec3(0.0f);
	mFront = glm::vec3(0.0f, 0.0f, 1.0f);
	mMinSpeed = 1.0f;
	mMaxSpeed = 10.0f;
	mAcceleration = glm::vec3(0.0f);
}

void Flock::Destroy()
{
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
	
	glm::vec3 front = glm::vec3(0.0f, 0.0f, 1.0f);
	if (glm::length(mVelocity) > 0.0f)
	{
		front = glm::normalize(mVelocity);
	}

	// The fish mesh (pyramid) points along +Y in model space, so its model
	// rotation has to take +Y onto the direction of travel.
	glm::quat orientation;
	if (glm::dot(Y, front) < -0.9999f)
	{
		// glm::rotation is undefined for exactly opposite vectors.
		orientation = glm::angleAxis(3.14159265358979323846f, glm::vec3(0.0f, 0.0f, 1.0f));
	}
	else
	{
		orientation = glm::rotation(Y, front);
	}

	trans->mRotationMatrix = toMat4(orientation);
	trans->SetModelTransformation();

	// Transform::Update() already uploaded this frame's matrices before this
	// component ran, so the new rotation has to be pushed to the UBO as well.
	trans->UpdateUniformBuffer();
}

void Flock::Serialize(rapidjson::Value::Object data)
{
}
