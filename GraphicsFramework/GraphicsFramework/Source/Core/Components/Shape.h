#pragma once
#include "Components.h"

class VertexArray;
class ElementArrayBuffer;
enum Shapes;

class Shape : public Component
{
	GF_COMPONENT(Shape);
public:
	std::pair<VertexArray*, ElementArrayBuffer*> mShapeData;
	Shapes mShape;

	void Update() override;

	void Serialize(rapidjson::Value::Object data) override;
};