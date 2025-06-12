#pragma once
#include "Singleton.h"
#include <vector>
#include <string>

class Object;
class Shader;

class ObjectManager : public Singleton<ObjectManager>
{
public:
	Object* AddObject(std::string path);
	static Object* ReadObject(std::string path);
	void UpdateObjects();
	void RenderObjects(Shader* shader);
	void Close();

	friend class Renderer;
private:
	std::vector<Object*> mObjects;
};
