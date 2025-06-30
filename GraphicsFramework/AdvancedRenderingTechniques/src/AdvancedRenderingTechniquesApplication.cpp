#include "AdvancedRenderingTechniquesApplication.h"

#include <GraphicsFramework.h>

// Projects
#include "AllProjects.h"
#include "MotionBlur.h"

Application* CreateApplication()
{
	return new AdvancedRenderingTechniquesApplication();
}

Settings AdvancedRenderingTechniquesApplication::GetSettings()
{
	Settings settings;
	settings.mWindowSize = { 1280, 720 };
	settings.mShadersListPath = "res/Shaders/ShadersList.ini";
	return settings;
}

void AdvancedRenderingTechniquesApplication::Init()
{
	Engine::Instance()->AddCustomDebugFunction(std::bind(&AdvancedRenderingTechniquesApplication::ProjectSelector, this));
}

void AdvancedRenderingTechniquesApplication::Close()
{

}

void AdvancedRenderingTechniquesApplication::ProjectSelector()
{
	ImGui::Begin("Projects");

	static const char* projectList[] = {
		"None",
		"AllProjects",
		"MotionBlur",
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
							Engine::Instance()->SetScene<AllProjects>();
							break;
						case 2:
							Engine::Instance()->SetScene<MotionBlur>();
							break;
					}
				}
			}
		}
		ImGui::EndCombo();
	}

	ImGui::End();
}
