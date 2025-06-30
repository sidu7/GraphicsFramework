#pragma once
#include <rapidjson/document.h>
#include "../ComponentManager.h"
#include <glm/glm.hpp>
#include <typeindex>
#include <string>

class Object;
class Shader;

#define COMPONENT(x) public:										\
					std::type_index GetIndex()						\
					{												\
						return std::type_index(typeid(x));			\
					}												\
					private:										\
					x* New()										\
					{												\
						return new x();								\
					}

class Component
{
public:
	Object* mOwner;

	Component() : mOwner(nullptr) {}
	Component(std::string name);
	virtual ~Component() {}
	
	virtual void Update() {}
	virtual void Serialize(rapidjson::Value::Object data) = 0;
	virtual Component* New() = 0;
	virtual std::type_index GetIndex() = 0;
};

inline Component::Component(std::string name)
{
	ComponentManager::Instance()->mComponentsMap[name] = this;
}
