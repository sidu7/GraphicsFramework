#include "Object.h"
#include "../utils/JSONHelper.h"
#include "ShapeManager.h"
#include "../opengl/Shader.h"
#include "Components/Components.h"
#include "../opengl/Renderer.h"
#include "../opengl/Texture.h"
#include "Components/Transform.h"
#include "Components/Material.h"
#include "Components/RotateInCircle.h"
#include "Components/Move.h"
#include "Components/Shape.h"

Object::Object()
{
}

Object::~Object()
{
	for(auto index : mIndices)
	{
		delete mComponents[index];
	}
}

void Object::Update(Shader* shader)
{
	for(auto index : mIndices)
	{
		mComponents[index]->Update(shader);
	}
}

void Object::Serialize(const rapidjson::Value::Object& data)
{
	rapidjson::Value::Array components = data["Components"].GetArray();
	for (unsigned int i = 0; i < components.Size(); ++i)
	{
		std::string type = components[i]["Type"].GetString();
		Component* component = ComponentManager::Instance().NewComponent(type);
		component->Serialize(components[i].GetObject());
		component->mOwner = this;
		AddComponent(component);		
	}
}

void Object::AddComponent(Component* component)
{
	auto index = component->GetIndex();
	mComponents[index] = component;
	mIndices.push_back(index);
}
