#include <iostream>
#include "Core/Engine.h"
#include "Flocking.h"

int main(int argc, char* args[])
{
	std::cout << "Graphics Framework" << std::endl;

	Engine::Instance()->Start("res/Settings.json");
	Engine::Instance()->SetScene<Flocking>();
	Engine::Instance()->Run();
	Engine::Instance()->Stop();

	//system("pause");
	return 0;
}
