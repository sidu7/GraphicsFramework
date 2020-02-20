#include <iostream>
#include <core/Engine.h>
#include "AnimationScene.h"
#include "Mechanics.h"

int main(int argc, char* args[])
{
	std::cout << "Graphics Framework" << std::endl;

	AnimationScene proj;
	//Mechanics proj;
	
	Engine::Instance().Start(&proj);
	Engine::Instance().Run();
	Engine::Instance().Stop();

	//system("pause");
	return 0;
}
