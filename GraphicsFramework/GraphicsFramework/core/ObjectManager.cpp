#include "ObjectManager.h"
#include <algorithm>
#include "Object.h"
#include "../opengl/Renderer.h"
#include "../opengl/Shader.h"
#include "../utils/JSONHelper.h"

Object* ObjectManager::AddObject(std::string path)
{
	PARSE_JSON_FILE(path);

	Object* object = new Object();
	object->Serialize(root.GetObject());
	mObjects.emplace_back(object);
	return object;
}

Object* ObjectManager::ReadObject(std::string path)
{
	PARSE_JSON_FILE(path);

	Object* object = new Object();
	object->Serialize(root.GetObject());
	return object;
}

void ObjectManager::ObjectsDraw(Shader* shader)
{
	shader->Bind();
	for (auto object : mObjects)
	{
		object->Update(shader);
		Renderer::Instance().DrawObject(shader,object);
	}
}

void ObjectManager::Close()
{
	std::for_each(mObjects.begin(), mObjects.end(), [](Object* x) { delete x; });
}
