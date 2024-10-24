#pragma once
#include "Components.h"

class Texture;

class Material : public Component
{
	GF_COMPONENT(Material);
public:
	glm::vec3 mDiffuse;
	glm::vec3 mDebugColor;
	glm::vec3 mSpecular;
	float mShininess;
	Texture* pTexture = nullptr;
	bool mWireMesh = false;
	bool mDebugMesh = false;
	bool mMesh = true;
	bool mLighting = true;

	void Update() override;	

	void Serialize(rapidjson::Value::Object data) override;	
};
