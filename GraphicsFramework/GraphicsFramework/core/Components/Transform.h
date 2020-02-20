#pragma once
#include "Components.h"

class Shader;

class Transform : public Component
{
public:
	glm::vec3 mPosition;
	glm::vec3 mScale;
	glm::vec3 mRotation;
	glm::mat4 mModelTransformation;
	glm::mat4 mPrevModelTransformation;

	void Update(Shader* shader) override;

	void  Serialize(rapidjson::Value::Object data) override;
};
