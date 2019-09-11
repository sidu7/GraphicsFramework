#include <iostream>
#include "Engine.h"

Engine* engine;

int main(int argc, char* args[])
{
	std::cout << "Graphics Framework" << std::endl;

	engine = new Engine();
	engine->Start();
	engine->Run();
	engine->Stop();

	system("pause");
	return 0;
}
