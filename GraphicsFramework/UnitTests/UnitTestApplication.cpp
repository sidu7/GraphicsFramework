#include "UnitTestApplication.h"

#include <GraphicsFramework.h>
#include "Rendering/ShaderRegistry.h"
#include "UnitTestShaderID.h"

// Projects
#include "ShaderTest.h"
#include "VertexBufferTest.h"
#include "IndexBufferTest.h"
#include "UniformBufferTest.h"
#include "TextureTest.h"

Application* CreateApplication()
{
	return new UnitTestApplication();
}

std::string UnitTestApplication::GetSettingsFilePath()
{
	return "res/Settings.json";
}

void UnitTestApplication::Init()
{
	ShaderRegistry::Instance()->Init("res/", [](ShaderRegistry* reg) {
		reg->RegisterShader(static_cast<int32_t>(UnitTestShaderID::SHADER_TEST),
			"ShaderTest.vert", "ShaderTest.frag",
			"spirv/ShaderTest.vert.spv", "spirv/ShaderTest.frag.spv");
		reg->RegisterShader(static_cast<int32_t>(UnitTestShaderID::VERTEXBUFFER_TEST),
			"VertexBufferTest.vert", "VertexBufferTest.frag",
			"spirv/VertexBufferTest.vert.spv", "spirv/VertexBufferTest.frag.spv");
		reg->RegisterShader(static_cast<int32_t>(UnitTestShaderID::INDEXBUFFER_TEST),
			"IndexBufferTest.vert", "IndexBufferTest.frag",
			"spirv/IndexBufferTest.vert.spv", "spirv/IndexBufferTest.frag.spv");
		reg->RegisterShader(static_cast<int32_t>(UnitTestShaderID::UNIFORMBUFFER_TEST),
			"UniformBufferTest.vert", "UniformBufferTest.frag",
			"spirv/UniformBufferTest.vert.spv", "spirv/UniformBufferTest.frag.spv");
		reg->RegisterShader(static_cast<int32_t>(UnitTestShaderID::TEXTURE_TEST),
			"TextureTest.vert", "TextureTest.frag",
			"spirv/TextureTest.vert.spv", "spirv/TextureTest.frag.spv");
	});

	Engine::Instance()->AddCustomDebugFunction(std::bind(&UnitTestApplication::UnitTestSelector, this));
	Engine::Instance()->SetScene<ShaderTest>();

	// Optional startup selector: -scene=<0..4> jumps straight to one unit test, which makes the
	// scenes scriptable for the backend verification (the combo below stays interactive).
	int SceneIndex = -1;
	if (CommandLine::Instance()->GetValue("-scene", SceneIndex))
	{
		SelectedUnitTest = SceneIndex;
		switch (SceneIndex)
		{
			case 0: Engine::Instance()->SetScene<ShaderTest>(); break;
			case 1: Engine::Instance()->SetScene<VertexBufferTest>(); break;
			case 2: Engine::Instance()->SetScene<IndexBufferTest>(); break;
			case 3: Engine::Instance()->SetScene<UniformBufferTest>(); break;
			case 4: Engine::Instance()->SetScene<TextureTest>(); break;
			default: break;
		}
	}
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
		"UniformBufferTest",
		"TextureTest"
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
						case 4:
							Engine::Instance()->SetScene<TextureTest>();
							break;
					}
				}
			}
		}
		ImGui::EndCombo();
	}

	ImGui::End();
}
