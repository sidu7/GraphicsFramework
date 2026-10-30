#include <iostream>

#include "Core/Engine.h"
#include "Wave.h"

int main(int argc, char* args[])
{
	std::cout << "Wave Simulation" << std::endl;

	Engine::Instance()->Start("res/Settings.json");
	Engine::Instance()->SetScene<Wave>();
	Engine::Instance()->Run();
	Engine::Instance()->Stop();

	return 0;
}
