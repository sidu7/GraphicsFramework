#pragma once
#include <glm/mat4x2.hpp>
#include <utility>
#include "JSONHelper.h"
#include "Time.h"
#include "Object.h"

enum Shapes;
class VertexArray;
class ElementArrayBuffer;

enum ComponentType
{
	TRANSFORM,
	MATERIAL,
	SHAPE,
	ROTATEINCIRCLE
};

struct Component
{
	Object* mOwner;
};

struct Transform : public Component
{
	glm::vec3 mPosition;
	glm::vec3 mScale;
	glm::vec3 mRotation;
	glm::mat4 mModelTransformation;

	void Update(Shader* shader)
	{

	}

	void Serialize(rapidjson::Value::Object data)
	{
		mPosition = JSONHelper::GetVec3F(data["Position"].GetArray());
		mScale = JSONHelper::GetVec3F(data["Scale"].GetArray());
		mRotation = JSONHelper::GetVec3F(data["Rotation"].GetArray());
	}
};

struct Material : public Component
{
	glm::vec3 mDiffuse;
	glm::vec3 mSpecular;
	float mShininess;

	void Update(Shader* shader)
	{

	}

	void Serialize(rapidjson::Value::Object data)
	{
		mDiffuse = JSONHelper::GetVec3F(data["Diffuse"].GetArray());
		mSpecular = JSONHelper::GetVec3F(data["Specular"].GetArray());
		mShininess = data["Shininess"].GetFloat();
	}
};

struct Shape : public Component
{
	std::pair<VertexArray*, ElementArrayBuffer*> mShapeData;
	Shapes mShape;

	void Update()
	{

	}

	void Serialize(rapidjson::Value::Object data)
	{
		mShape = (Shapes)data["Shape"].GetUint();
		mShapeData = ShapeManager::Instance().mShapes[mShape];
	}
};

struct RotateInCircle : public Component
{
	float mTParam; // Position on circumference of circle
	float mAngle; // Angular rotation around itself
	float mRotationFactor;
	float mRadius;
	float mSpeed;
	
	void Update()
	{
		mAngle += Time::Instance().deltaTime * mRotationFactor;
		mTParam += Time::Instance().deltaTime;
		mOwner->pTransform->mPosition.x = mRadius * glm::cos(mTParam * mSpeed);
		mOwner->pTransform->mPosition.z = mRadius * glm::sin(mTParam * mSpeed);
	}

	void Serialize(rapidjson::Value::Object data)
	{
		mRadius = data["Radius"].GetFloat();
		mRotationFactor = data["RotationFactor"].GetFloat();
		mSpeed = data["Speed"].GetFloat();
		mTParam = 0.0f;
		mAngle = 0.0f;
	}
};