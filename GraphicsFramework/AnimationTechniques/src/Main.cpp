#include <iostream>
#include <core/Engine.h>
#include "AnimationScene.h"
#include "Mechanics.h"

int SelectedProject = 0;

void ProjectSelector()
{
	ImGui::Begin("Projects");

	static const char* projectList[] = {
		"None",
		"Animation",
		"Mechanics",
	};

	bool selected = true;
	if (ImGui::BeginCombo("Select Project", projectList[SelectedProject]))
	{
		for (int i = 0; i < ARRAY_SIZE(projectList); ++i)
		{
			if (ImGui::Selectable(projectList[i], selected))
			{
				if (SelectedProject != i)
				{
					SelectedProject = i;
					switch (SelectedProject)
					{
						case 1:
							Engine::Instance()->SetScene<AnimationScene>();
							break;
						case 2:
							Engine::Instance()->SetScene<Mechanics>();
							break;
					}
				}
			}
		}
		ImGui::EndCombo();
	}

	ImGui::End();
}

int main(int argc, char* args[])
{
	std::cout << "Graphics Framework" << std::endl;

	Engine::Instance()->Start(1280,720);
	Engine::Instance()->SetScene<AnimationScene>();
	Engine::Instance()->Run();
	Engine::Instance()->Stop();

	//system("pause");
	return 0;
}
