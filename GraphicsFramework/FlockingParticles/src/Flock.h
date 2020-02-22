#pragma once
#include "core/Components/Components.h"

class Flock : public Component
{
	COMPONENT(Flock);
public:
	void Update(Shader* shader) override;
	void Serialize(rapidjson::Value::Object data) override;

	glm::vec3 mNormal;
	glm::vec3 mVelocity;
	glm::vec3 mAcceleration;
};