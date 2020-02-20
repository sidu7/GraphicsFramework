#pragma once
#include "Components.h"

class RotateInCircle : public Component
{
public:
	float mTParam; // Position on circumference of circle
	float mAngle; // Angular rotation around itself
	float mRotationFactor;
	float mRadius;
	float mSpeed;

	void Update(Shader* shader) override;

	void Serialize(rapidjson::Value::Object data) override;
};
