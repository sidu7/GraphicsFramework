#include <iostream>
#include <core/Engine.h>
#include "RayTracing.h"

int main(int argc, char* args[])
{
	std::cout << "Graphics Framework" << std::endl;

	RayTracing proj;

	Engine::Instance().Start(&proj,400,300);
	Engine::Instance().Run();
	Engine::Instance().Stop();

	//system("pause");
	return 0;
}
