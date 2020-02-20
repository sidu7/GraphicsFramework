#include "Shape.h"
#include "../ShapeManager.h"

void Shape::Update(Shader* shader)
{
}

void Shape::Serialize(rapidjson::Value::Object data)
{
	mShape = (Shapes)data["Shape"].GetUint();
	mShapeData = ShapeManager::Instance().mShapes[mShape];
}
