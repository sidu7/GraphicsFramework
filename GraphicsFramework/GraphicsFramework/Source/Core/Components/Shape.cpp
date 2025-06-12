#include "Shape.h"
#include "Core/ShapeManager.h"
#include "Rendering/Renderer.h"

Shape::Shape() :
	mShapeData(nullptr),
	mShape(SHAPES_NUM),
	mMesh(true),
	mWireMesh(false),
	mDebugMesh(false)
{ }

void Shape::Serialize(rapidjson::Value::Object data)
{
	if (data.HasMember("Shape"))
	{
		mShape = (Shapes)data["Shape"].GetUint();
		mShapeData = &ShapeManager::Instance()->mShapes[mShape];
	}
	if (data.HasMember("WireMesh"))
	{
		mWireMesh = data["WireMesh"].GetBool();
	}
	if (data.HasMember("DebugMesh"))
	{
		mDebugMesh = data["DebugMesh"].GetBool();
	}
	if (data.HasMember("Mesh"))
	{
		mMesh = data["Mesh"].GetBool();
	}
}
