#pragma once
#include "Components.h"

class VertexArray;
class IndexBuffer;
enum Shapes;
struct ShapeData;

class Shape : public Component
{
	COMPONENT(Shape);
public:
	Shape();

	ShapeData* mShapeData;
	Shapes mShape;

	bool mWireMesh;
	bool mDebugMesh;
	bool mMesh;

	void Serialize(rapidjson::Value::Object data) override;
};