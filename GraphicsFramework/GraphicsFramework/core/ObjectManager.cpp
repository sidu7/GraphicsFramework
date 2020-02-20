#include "ObjectManager.h"
#include "JSONHelper.h"
#include <algorithm>
#include "Object.h"

void ObjectManager::AddObject(std::string path)
{
	PARSE_JSON_FILE(path);

	Object* object = new Object();
	object->Serialize(root.GetObject());
	mObjects.emplace_back(object);
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
	for (auto object : mObjects)
	{
		object->Update(shader);
	}
}

void ObjectManager::Close()
{
	std::for_each(mObjects.begin(), mObjects.end(), [](Object* x) { delete x; });
}
