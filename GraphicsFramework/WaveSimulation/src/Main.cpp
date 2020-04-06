#include <iostream>
#include <core/Engine.h>
#include "Wave.h"

int main(int argc, char* args[])
{
	std::cout << "Wave Simulation" << std::endl;

	Wave proj;

	Engine::Instance().Start(&proj, 1280, 720);
	Engine::Instance().Run();
	Engine::Instance().Stop();

	//system("pause");
	return 0;
}
