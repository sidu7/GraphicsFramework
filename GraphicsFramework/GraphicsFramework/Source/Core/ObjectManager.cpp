#include "ObjectManager.h"

#include "Core/Core.h"
#include "Core/Engine.h"
#include "Object.h"
#include "Core/Scene.h"
#include "Rendering/Renderer.h"
#include "Rendering/Shader.h"
#include "Utils/JSONHelper.h"

Object* ObjectManager::AddObject(std::string path)
{
	auto root = JSONHelper::ParseFile(path);
	Object* object = new Object();
	object->Serialize(root.GetObject());
	mObjects.emplace_back(object);
	return object;
}

Object* ObjectManager::ReadObject(std::string path)
{
	auto root = JSONHelper::ParseFile(path);
	Object* object = new Object();
	object->Serialize(root.GetObject());
	return object;
}

void ObjectManager::UpdateObjects()
{
	for (Object* object : mObjects)
	{
		object->Update();
	}
}

void ObjectManager::RenderObjects(Shader* shader)
{
	for (Object* object : mObjects)
	{
		Engine::Instance()->GetScene()->RenderObject(object, shader);
	}
}

void ObjectManager::Close()
{
	ClearObjects();
}

void ObjectManager::ClearObjects()
{
	std::for_each(mObjects.begin(), mObjects.end(), [](Object* x) { delete x; });
	mObjects.clear();
}
