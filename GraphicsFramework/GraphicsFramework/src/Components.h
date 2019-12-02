#pragma once
#include <glm/mat4x2.hpp>
#include <utility>
#include "JSONHelper.h"
#include "Time.h"
#include "Object.h"
#include "glm/gtc/matrix_transform.hpp"
#include "../opengl/Texture.h"
#include "ShapeManager.h"
#include "Engine.h"

enum Shapes;
class VertexArray;
class ElementArrayBuffer;
class Object;

extern Engine* engine;

enum ComponentType
{
	TRANSFORM,
	MATERIAL,
	SHAPE,
	ROTATEINCIRCLE,
	MOVE
};

class Component
{
public:
	Object* mOwner;
};

class Transform : public Component
{
public:
	glm::vec3 mPosition;
	glm::vec3 mScale;
	glm::vec3 mRotation;
	glm::mat4 mModelTransformation;
	glm::mat4 mPrevModelTransformation;

	void Update(Shader* shader)
	{
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, mPosition);
		model = glm::rotate(model, glm::radians(mRotation.z), glm::vec3(0.0, 0.0, 1.0));
		model = glm::rotate(model, glm::radians(mRotation.y), glm::vec3(0.0, 1.0, 0.0));
		model = glm::rotate(model, glm::radians(mRotation.x), glm::vec3(1.0, 0.0, 0.0));
		model = glm::scale(model, mScale);
		mPrevModelTransformation = mModelTransformation;
		mModelTransformation = model;
		shader->SetUniformMat4f("model", mModelTransformation);
		shader->SetUniformMat4f("prevmodel", mPrevModelTransformation);
		shader->SetUniformMat4f("normaltr", glm::inverse(mModelTransformation));
	}

	void Serialize(rapidjson::Value::Object data)
	{
		mPosition = JSONHelper::GetVec3F(data["Position"].GetArray());
		mScale = JSONHelper::GetVec3F(data["Scale"].GetArray());
		mRotation = JSONHelper::GetVec3F(data["Rotation"].GetArray());
		mModelTransformation = glm::mat4(1.0f);
	}
};

class Material : public Component
{
public:
	glm::vec3 mDiffuse;
	glm::vec3 mSpecular;
	float mShininess;
	Texture* pTexture;

	void Update(Shader* shader)
	{
		shader->SetUniform3f("diffuse", mDiffuse.x, mDiffuse.y, mDiffuse.z);
		shader->SetUniform3f("specular", mSpecular.x, mSpecular.y, mSpecular.z);
		shader->SetUniform1f("shininess", mShininess);
	}

	void Serialize(rapidjson::Value::Object data)
	{
		if (data.HasMember("Diffuse"))
		{
			mDiffuse = JSONHelper::GetVec3F(data["Diffuse"].GetArray());
			pTexture = nullptr;
		}
		else
		{
			pTexture = new Texture(data["Texture"].GetString());
			mDiffuse = glm::vec3(0.0f);
		}
		mSpecular = JSONHelper::GetVec3F(data["Specular"].GetArray());
		mShininess = data["Shininess"].GetFloat();
	}
};

class Shape : public Component
{
public:
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

class RotateInCircle : public Component
{
public:
	float mTParam; // Position on circumference of circle
	float mAngle; // Angular rotation around itself
	float mRotationFactor;
	float mRadius;
	float mSpeed;
	
	void Update()
	{
		if(engine->stopMoving)
		{
			return;
		}		
		mAngle += Time::Instance().deltaTime * mRotationFactor;
		mTParam += Time::Instance().deltaTime;
		mOwner->pTransform->mPosition.x = mRadius * glm::cos(mTParam * mSpeed);
		mOwner->pTransform->mPosition.z = mRadius * glm::sin(mTParam * mSpeed);
		mOwner->pTransform->mRotation.y = mAngle;
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

class Move : public Component
{
public:
	float mTime;
	float mSpeed;
	bool mHorizontal;
	float mCurrentTime;
	bool mFlip;

	void Update()
	{
		float deltaTime = Time::Instance().deltaTime;
		mCurrentTime += deltaTime;
		if(mHorizontal)
		{
			if (mFlip)
			{
				mOwner->pTransform->mPosition.x += mSpeed * deltaTime;
			}
			else
			{
				mOwner->pTransform->mPosition.x -= mSpeed * deltaTime;
			}
		}
		else
		{
			if (mFlip)
			{
				mOwner->pTransform->mPosition.y += mSpeed * deltaTime;
			}
			else
			{
				mOwner->pTransform->mPosition.y -= mSpeed * deltaTime;
			}
		}
		if(mCurrentTime >= mTime)
		{
			mFlip = !mFlip;
			mCurrentTime = 0.0f;
		}
	}

	void Serialize(rapidjson::Value::Object data)
	{
		mTime = data["Time"].GetFloat();
		mSpeed = data["Speed"].GetFloat();
		mHorizontal = data["Horizontal"].GetBool();
		mCurrentTime = 0.0f;
		mFlip = data["Flip"].GetBool();
	}
};