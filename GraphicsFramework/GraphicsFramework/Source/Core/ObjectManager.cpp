#include "ObjectManager.h"

#include "Core/Core.h"
#include "Core/Engine.h"
#include "Object.h"
#include "Core/Scene.h"
#include "Rendering/Renderer.h"
#include "Rendering/Shader.h"
#include "Utils/JSONHelper.h"
#include "Profiling/Profiler.h"

#include <filesystem>

namespace
{
	std::string ObjectNameFromPath(const std::string& path)
	{
		return std::filesystem::path(path).stem().string();
	}
}

Object* ObjectManager::AddObject(std::string path)
{
	auto root = JSONHelper::ParseFile(path);
	Object* object = new Object();
	object->Serialize(root.GetObject());
	if (object->GetName().empty())
	{
		object->SetName(ObjectNameFromPath(path));
	}
	mObjects.emplace_back(object);
	return object;
}

Object* ObjectManager::ReadObject(std::string path)
{
	auto root = JSONHelper::ParseFile(path);
	Object* object = new Object();
	object->Serialize(root.GetObject());
	if (object->GetName().empty())
	{
		object->SetName(ObjectNameFromPath(path));
	}
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
		// Tag the draw calls issued by this object so the profiler can report
		// which object each draw belongs to.
		Profiler::Instance()->SetDrawCallObjectName(object->GetName());
		Engine::Instance()->GetScene()->RenderObject(object, shader);
	}
	Profiler::Instance()->SetDrawCallObjectName("");
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
