#include "UnitTestApplication.h"

#include <GraphicsFramework.h>

// Projects
#include "ShaderTest.h"
#include "VertexBufferTest.h"
#include "IndexBufferTest.h"
#include "UniformBufferTest.h"

Application* CreateApplication()
{
	return new UnitTestApplication();
}

std::string UnitTestApplication::GetSettingsFilePath()
{
	return "Settings.json";
}

void UnitTestApplication::Init()
{
	Engine::Instance()->AddCustomDebugFunction(std::bind(&UnitTestApplication::UnitTestSelector, this));
	Engine::Instance()->SetScene<ShaderTest>();
}

void UnitTestApplication::Close()
{

}

void UnitTestApplication::UnitTestSelector()
{
	ImGui::Begin("Projects");

	static const char* projectList[] = {
		"ShaderTest",
		"VertexBufferTest",
		"IndexBufferTest",
		"UniformBufferTest"
	};

	bool selected = true;
	if (ImGui::BeginCombo("Select Unit Test", projectList[SelectedUnitTest]))
	{
		for (int i = 0; i < ARRAY_SIZE(projectList); ++i)
		{
			if (ImGui::Selectable(projectList[i], selected))
			{
				if (SelectedUnitTest != i)
				{
					SelectedUnitTest = i;
					switch (SelectedUnitTest)
					{
						case 0:
							Engine::Instance()->SetScene<ShaderTest>();
							break;
						case 1:
							Engine::Instance()->SetScene<VertexBufferTest>();
							break;
						case 2:
							Engine::Instance()->SetScene<IndexBufferTest>();
							break;
						case 3:
							Engine::Instance()->SetScene<UniformBufferTest>();
							break;
					}
				}
			}
		}
		ImGui::EndCombo();
	}

	ImGui::End();
}