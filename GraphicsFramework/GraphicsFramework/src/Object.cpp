#include "Object.h"
#include "JSONHelper.h"
#include "ShapeManager.h"

void Object::Update()
{
}

void Object::Serialize(const rapidjson::Value::Object& data)
{
	mPosition = JSONHelper::GetVec3F(data["Position"].GetArray());
	mScale = JSONHelper::GetVec3F(data["Scale"].GetArray());
	mRotation = JSONHelper::GetVec3F(data["Rotation"].GetArray());
	mDiffuse = JSONHelper::GetVec3F(data["Diffuse"].GetArray());
	mSpecular = JSONHelper::GetVec3F(data["Specular"].GetArray());
	mShininess = data["Shininess"].GetFloat();
	mShape = (Shapes)data["Shape"].GetUint();
	mShapeData = ShapeManager::Instance().mShapes[mShape];
}
