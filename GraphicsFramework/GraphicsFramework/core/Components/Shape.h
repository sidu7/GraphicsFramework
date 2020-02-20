#pragma once
#include "Components.h"

class VertexArray;
class ElementArrayBuffer;
enum Shapes;

class Shape : public Component
{
public:
	std::pair<VertexArray*, ElementArrayBuffer*> mShapeData;
	Shapes mShape;

	void Update(Shader* shader) override;

	void Serialize(rapidjson::Value::Object data) override;
};