#include <iostream>
#include "Core/Engine.h"
#include "Flocking.h"

int main(int argc, char* args[])
{
	std::cout << "Graphics Framework" << std::endl;

	Flocking proj;
	
	Engine::Instance()->Start(&proj,1280,720);
	Engine::Instance()->Run();
	Engine::Instance()->Stop();

	//system("pause");
	return 0;
}
