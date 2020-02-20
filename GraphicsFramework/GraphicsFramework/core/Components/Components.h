#pragma once
#include <rapidjson/document.h>
#include <glm/glm.hpp>

enum ComponentType
{
	TRANSFORM,
	MATERIAL,
	SHAPE,
	ROTATEINCIRCLE,
	MOVE
};

class Object;
class Shader;

class Component
{
public:
	Object* mOwner;

	virtual void Update(Shader* shader) = 0;
	virtual void Serialize(rapidjson::Value::Object data) = 0;

	template<typename T>
	static T* New()
	{
		return new T();
	}
};