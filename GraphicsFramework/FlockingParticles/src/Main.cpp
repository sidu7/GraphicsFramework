#include <iostream>
#include <core/Engine.h>
#include "Flocking.h"

int main(int argc, char* args[])
{
	std::cout << "Graphics Framework" << std::endl;

	Flocking proj;
	
	Engine::Instance().Start(&proj);
	Engine::Instance().Run();
	Engine::Instance().Stop();

	//system("pause");
	return 0;
}
