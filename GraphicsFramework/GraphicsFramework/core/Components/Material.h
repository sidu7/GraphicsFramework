#pragma once
#include "Components.h"

class Texture;

class Material : public Component
{
	GF_COMPONENT(Material);
public:
	glm::vec3 mDiffuse;
	glm::vec3 mSpecular;
	float mShininess;
	Texture* pTexture;
	bool mWireMesh = false;
	bool mDebugMesh = false;
	bool mLighting = false;

	void Update(Shader* shader) override;	

	void Serialize(rapidjson::Value::Object data) override;	
};
