#pragma once
#include <rapidjson/document.h>
#include <vector>
#include <typeindex>
#include <unordered_map>

class VertexArray;
class ElementArrayBuffer;
enum Shapes;
class Shader;
class Component;

class Object
{
public:
	Object();
	~Object();
	void Update(Shader* shader);
	void Serialize(const rapidjson::Value::Object& data);

	void AddComponent(Component* component);

	template<typename T>
	T* GetComponent();
	
public:
	std::unordered_map<std::type_index, Component*> mComponents;
	std::vector<std::type_index> mIndices;
};

template <typename T>
T* Object::GetComponent()
{
	if (mComponents.find(std::type_index(typeid(T))) != mComponents.end())
	{
		return static_cast<T*>(mComponents[std::type_index(typeid(T))]);
	}
	else
	{
		return nullptr;
	}
}
