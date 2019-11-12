#pragma once
#include <glm/mat4x2.hpp>
#include <utility>
#include <rapidjson/document.h>

class VertexArray;
class ElementArrayBuffer;
enum Shapes;

class Object
{
public:
	void Update();
	void Serialize(const rapidjson::Value::Object& data);
	
public:
	glm::vec3 mPosition;
	glm::vec3 mScale;
	glm::vec3 mRotation;
	glm::mat4 mModelTransformation;

	glm::vec3 mDiffuse;
	glm::vec3 mSpecular;
	float mShininess;

	std::pair<VertexArray*, ElementArrayBuffer*> mShapeData;
	Shapes mShape;
};
