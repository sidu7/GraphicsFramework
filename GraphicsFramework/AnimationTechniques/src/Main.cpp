#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>

#include "Core/Engine.h"
#include "AnimationScene.h"
#include "Mechanics.h"

namespace
{
	// -scene=1 -> AnimationScene (skinned archer)
	// -scene=2 -> Mechanics    (rope of cubes)
	// default  -> AnimationScene
	//
	// CommandLine::GetValue cannot be used: it returns true when the flag is
	// absent (Core/CommandLine.cpp compares against CmdStr.size() instead of
	// npos), and sample mains never call CommandLine::Init.
	int ParseSceneArgument(int argc, char* args[])
	{
		const std::string Prefix = "-scene=";

		for (int i = 1; i < argc; ++i)
		{
			const std::string Argument = args[i];
			if (Argument.rfind(Prefix, 0) == 0)
			{
				const std::string Value = Argument.substr(Prefix.size());
				if (!Value.empty())
				{
					return std::atoi(Value.c_str());
				}
			}
		}

		return 1;
	}
}

int main(int argc, char* args[])
{
	std::cout << "Animation Techniques" << std::endl;

	const int SceneIndex = ParseSceneArgument(argc, args);

	Engine::Instance()->Start("res/Settings.json");

	if (SceneIndex == 2)
	{
		Engine::Instance()->SetScene<Mechanics>();
	}
	else
	{
		Engine::Instance()->SetScene<AnimationScene>();
	}

	Engine::Instance()->Run();
	Engine::Instance()->Stop();

	return 0;
}
