#pragma once
#include "core/Components/Components.h"

struct NeighboursData
{
	glm::vec3 mFlockHeading;
	glm::vec3 mFlockCenter;
	glm::vec3 mAvoidanceDirection;

	NeighboursData();
	void Clear();
};

class Flock : public Component
{
	COMPONENT(Flock);
public:
	void Update() override;
	void Serialize(rapidjson::Value::Object data) override;

	glm::vec3 mNormal;
	glm::vec3 mVelocity;
	glm::vec3 mPreviousVelocity;
	glm::vec3 mFront;
	float mMinSpeed, mMaxSpeed;
	glm::vec3 mAcceleration;
	NeighboursData mNeighboursData;
};