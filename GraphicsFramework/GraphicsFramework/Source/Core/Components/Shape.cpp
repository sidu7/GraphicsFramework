#include "Shape.h"
#include "../ShapeManager.h"
#include "../../opengl/Renderer.h"

void Shape::Update()
{
	Renderer::Instance().GetRenderData().mShape = this;
}

void Shape::Serialize(rapidjson::Value::Object data)
{
	mShape = (Shapes)data["Shape"].GetUint();
	mShapeData = ShapeManager::Instance().mShapes[mShape];
}
