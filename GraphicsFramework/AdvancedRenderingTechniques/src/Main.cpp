#include <iostream>
#include <core/Engine.h>
#include "AllProjects.h"
#include "MotionBlur.h"

int main(int argc, char* args[])
{
	std::cout << "Graphics Framework" << std::endl;

	AllProjects proj;
	//MotionBlur proj;
	
	Engine::Instance().Start(&proj);
	Engine::Instance().Run();
	Engine::Instance().Stop();

	//system("pause");
	return 0;
}
