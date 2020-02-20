#pragma once
#include "Components.h"

class Texture;

class Material : public Component
{
public:
	glm::vec3 mDiffuse;
	glm::vec3 mSpecular;
	float mShininess;
	Texture* pTexture;

	void Update(Shader* shader) override;	

	void Serialize(rapidjson::Value::Object data) override;	
};
