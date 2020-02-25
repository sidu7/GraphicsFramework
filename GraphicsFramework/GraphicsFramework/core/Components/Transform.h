#pragma once
#include "Components.h"

class Shader;

class Transform : public Component
{
	GF_COMPONENT(Transform);
public:
	glm::vec3 mPosition;
	glm::vec3 mScale;
	glm::vec3 mRotation;
	glm::vec3 mFront;
	glm::mat4 mTranslationMatrix;
	glm::mat4 mRotationMatrix;
	glm::mat4 mScaleMatrix;
	glm::mat4 mModelTransformation;
	glm::mat4 mPrevModelTransformation;

	void Update(Shader* shader) override;

	void  Serialize(rapidjson::Value::Object data) override;

	void SetModelTransformation(Shader* shader);
};
