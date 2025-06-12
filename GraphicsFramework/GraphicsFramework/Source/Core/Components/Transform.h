#pragma once
#include "Components.h"

class Shader;
class UniformBuffer;

struct ObjectMatricesUBO
{
	glm::mat4 mModelTr;
	glm::mat4 mNormalTr;
	glm::mat4 mPrevModelTr;
};

class Transform : public Component
{
	COMPONENT(Transform);
public:
	Transform();
	~Transform();

	glm::vec3 mPosition;
	glm::vec3 mScale;
	glm::vec3 mRotation;
	glm::vec3 mFront;
	glm::mat4 mTranslationMatrix;
	glm::mat4 mRotationMatrix;
	glm::mat4 mScaleMatrix;
	glm::mat4 mModelTransformation;
	glm::mat4 mPrevModelTransformation;

	UniformBuffer* mMatricesUBO;

	void Update() override;

	void Serialize(rapidjson::Value::Object data) override;

	void SetModelTransformation();
};
