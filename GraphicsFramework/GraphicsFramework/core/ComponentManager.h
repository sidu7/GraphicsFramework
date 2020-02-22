#pragma once
#include "Singleton.h"
#include <unordered_map>

class Component;

class ComponentManager
{
	SINGLETON(ComponentManager);
public:
	void Init();
	void Close();
	Component* NewComponent(std::string cName);
	
	friend class Component;
private:
	std::unordered_map<std::string, Component*> mComponentsMap;
};
