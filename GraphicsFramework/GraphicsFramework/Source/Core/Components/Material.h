#pragma once
#include "Components.h"

class Texture;
class UniformBuffer;

struct MaterialDataUBO
{
	glm::vec4 mDiffuse;
	glm::vec4 mSpecular;
	float mShininess;
	bool mLighting;
};

class Material : public Component
{
	COMPONENT(Material);
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
