#pragma once
#include <rapidjson/document.h>

class Transform;
class Material;
class Shape;
class RotateInCircle;
class Move;

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
	Move* pMove;
};
