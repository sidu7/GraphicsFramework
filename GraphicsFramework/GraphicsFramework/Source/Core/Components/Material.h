#pragma once
#include "Components.h"

class Texture;
class UniformBuffer;

// std140 layout: vec4s are 16-byte aligned, the scalars after them are packed at
// their natural 4-byte alignment (shininess at 32, lighting at 36). Using an int
// for the flag keeps the four bytes that the GLSL side reads well defined.
struct MaterialDataUBO
{
	glm::vec4 mDiffuse;
	glm::vec4 mSpecular;
	float mShininess;
	int mLighting;
};

class Material : public Component
{
public:
	glm::vec3 mDiffuse;
	glm::vec3 mDebugColor;
	glm::vec3 mSpecular;
	float mShininess;
	Texture* pTexture;
	bool mLighting;
	UniformBuffer* mMaterialUBO;

	Material();
	virtual ~Material();

	virtual void Update() override;

	virtual void Serialize(rapidjson::Value::Object data) override;	
};
