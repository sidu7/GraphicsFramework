#pragma once
#include <rapidjson/document.h>
#include "../ComponentManager.h"
#include <glm/glm.hpp>
#include <typeindex>

class Object;
class Shader;

#define COMPONENT(x) public:										\
					x() : Component(#x) {}							\
					std::type_index GetIndex()						\
					{												\
						return std::type_index(typeid(x));			\
					}												\
					private:										\
					static x instance;								\
					x* New()										\
					{												\
						return new x();								\
					}

#define GF_COMPONENT(x) public:										\
					x() : Component() {}							\
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

	Component() = default;
	Component(std::string name);
	
	virtual void Update(Shader* shader) = 0;
	virtual void Serialize(rapidjson::Value::Object data) = 0;
	virtual Component* New() = 0;
	virtual std::type_index GetIndex() = 0;
};

inline Component::Component(std::string name)
{
	ComponentManager::Instance().mComponentsMap[name] = this;
}
