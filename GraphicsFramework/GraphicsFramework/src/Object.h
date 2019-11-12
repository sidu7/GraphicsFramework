#pragma once
#include <rapidjson/document.h>
#include "Components.h"

class VertexArray;
class ElementArrayBuffer;
enum Shapes;
class Shader;

class Object
{
public:
	Object();
	~Object();
	void Update(Shader* shader);
	void Serialize(const rapidjson::Value::Object& data);
	
public:
	Transform* pTransform;
	Material* pMaterial;
	Shape* pShape;
	RotateInCircle* pRotate;
};
