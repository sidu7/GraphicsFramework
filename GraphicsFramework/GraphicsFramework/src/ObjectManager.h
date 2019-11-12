#pragma once
#include "Singleton.h"
#include <vector>
#include <string>

class Object;
class Shader;

class ObjectManager
{
	SINGLETON(ObjectManager);
public:
	void AddObject(std::string path);
	void ObjectsDraw(Shader* shader);
	void Close();
private:
	std::vector<Object*> mObjects;
};
