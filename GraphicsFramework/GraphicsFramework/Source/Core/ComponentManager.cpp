#include "ComponentManager.h"

#include "Components/Components.h"
#include "Components/Transform.h"
#include "Components/Material.h"
#include "Components/Shape.h"
#include "Components/RotateInCircle.h"
#include "Components/Move.h"
#include <iostream>

void ComponentManager::Init()
{
	mComponentsMap["Transform"] = new Transform();
	mComponentsMap["Material"] = new Material();
	mComponentsMap["Shape"] = new Shape();
	mComponentsMap["Move"] = new Move();
	mComponentsMap["RotateInCircle"] = new RotateInCircle();
}

void ComponentManager::Close()
{
	delete mComponentsMap["Transform"];
	delete mComponentsMap["Material"];
	delete mComponentsMap["Shape"];
	delete mComponentsMap["Move"];
	delete mComponentsMap["RotateInCircle"];
	mComponentsMap.clear();
}

Component* ComponentManager::NewComponent(std::string cName)
{
	if(mComponentsMap.find(cName) == mComponentsMap.end())
	{
		std::cout << "Component " << cName << " not found!" << std::endl;
		__debugbreak();
	}
	return mComponentsMap[cName]->New();
}
