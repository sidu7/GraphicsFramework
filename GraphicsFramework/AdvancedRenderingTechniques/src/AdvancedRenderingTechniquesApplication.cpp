#include "AdvancedRenderingTechniquesApplication.h"

#include <GraphicsFramework.h>
#include "Rendering/ShaderRegistry.h"

// Projects
#include "AllProjects.h"
#include "MotionBlur.h"

Application* CreateApplication()
{
	return new AdvancedRenderingTechniquesApplication();
}

std::string AdvancedRenderingTechniquesApplication::GetSettingsFilePath()
{
	return "res/Settings.json";
}

void AdvancedRenderingTechniquesApplication::Init()
{
	ShaderRegistry::Instance()->Init("res/shaders/", [](ShaderRegistry* reg) {
		// AllProjects shaders
		reg->RegisterShader(static_cast<int32_t>(AppShaderID::DRAWING),
			"Drawing.vert", "Drawing.frag",
			"spirv/Drawing.vert.spv", "spirv/Drawing.frag.spv");
		reg->RegisterShader(static_cast<int32_t>(AppShaderID::LIGHTING),
			"Lighting.vert", "Lighting.frag",
			"spirv/Lighting.vert.spv", "spirv/Lighting.frag.spv");
		reg->RegisterShader(static_cast<int32_t>(AppShaderID::AMBIENT),
			"Ambient.vert", "Ambient.frag",
			"spirv/Ambient.vert.spv", "spirv/Ambient.frag.spv");
		reg->RegisterShader(static_cast<int32_t>(AppShaderID::AMBIENT_NO_AO),
			"Ambient.vert", "AmbientNoAO.frag",
			"spirv/Ambient.vert.spv", "spirv/AmbientNoAO.frag.spv");
		reg->RegisterShader(static_cast<int32_t>(AppShaderID::SHADOW),
			"Shadow.vert", "Shadow.frag",
			"spirv/Shadow.vert.spv", "spirv/Shadow.frag.spv");
		reg->RegisterShader(static_cast<int32_t>(AppShaderID::LOCAL_LIGHT),
			"LocalLight.vert", "LocalLight.frag",
			"spirv/LocalLight.vert.spv", "spirv/LocalLight.frag.spv");
		reg->RegisterShader(static_cast<int32_t>(AppShaderID::SKYDOME),
			"SkyDome.vert", "SkyDome.frag",
			"spirv/SkyDome.vert.spv", "spirv/SkyDome.frag.spv");
		reg->RegisterShader(static_cast<int32_t>(AppShaderID::AMBIENT_OCCLUSION),
			"AO.vert", "AO.frag",
			"spirv/AO.vert.spv", "spirv/AO.frag.spv");
		reg->RegisterShader(static_cast<int32_t>(AppShaderID::BLUR),
			"Blur.vert", "Blur.frag",
			"spirv/Blur.vert.spv", "spirv/Blur.frag.spv");

		// Compute shaders
		reg->RegisterShader(static_cast<int32_t>(AppComputeID::BLUR_HORIZONTAL),
			"BlurHorizontal.compute", "",
			"spirv/BlurHorizontal.compute.spv", "");
		reg->RegisterShader(static_cast<int32_t>(AppComputeID::BLUR_VERTICAL),
			"BlurVertical.compute", "",
			"spirv/BlurVertical.compute.spv", "");
		reg->RegisterShader(static_cast<int32_t>(AppComputeID::BILATERAL_HORIZONTAL),
			"BilateralHorizontal.compute", "",
			"spirv/BilateralHorizontal.compute.spv", "");
		reg->RegisterShader(static_cast<int32_t>(AppComputeID::BILATERAL_VERTICAL),
			"BilateralVertical.compute", "",
			"spirv/BilateralVertical.compute.spv", "");

		// MotionBlur shaders (in motionblur/ subdirectory)
		reg->RegisterShader(static_cast<int32_t>(MotionBlurShaderID::GBUFFER),
			"motionblur/GBuffer.vert", "motionblur/GBuffer.frag",
			"spirv/motionblur/GBuffer.vert.spv", "spirv/motionblur/GBuffer.frag.spv");
		reg->RegisterShader(static_cast<int32_t>(MotionBlurShaderID::MOTIONBLUR_AMBIENT),
			"motionblur/Ambient.vert", "motionblur/Ambient.frag",
			"spirv/motionblur/Ambient.vert.spv", "spirv/motionblur/Ambient.frag.spv");
		reg->RegisterShader(static_cast<int32_t>(MotionBlurShaderID::MOTIONBLUR_LIGHTING),
			"motionblur/Lighting.vert", "motionblur/Lighting.frag",
			"spirv/motionblur/Lighting.vert.spv", "spirv/motionblur/Lighting.frag.spv");
		reg->RegisterShader(static_cast<int32_t>(MotionBlurShaderID::SKYDOME),
			"SkyDome.vert", "SkyDome.frag",
			"spirv/SkyDome.vert.spv", "spirv/SkyDome.frag.spv");
		reg->RegisterShader(static_cast<int32_t>(MotionBlurShaderID::TILE_MAX),
			"motionblur/TileMax.vert", "motionblur/TileMax.frag",
			"spirv/motionblur/TileMax.vert.spv", "spirv/motionblur/TileMax.frag.spv");
		reg->RegisterShader(static_cast<int32_t>(MotionBlurShaderID::NEIGHBOUR_MAX),
			"motionblur/NeighbourMax.vert", "motionblur/NeighbourMax.frag",
			"spirv/motionblur/NeighbourMax.vert.spv", "spirv/motionblur/NeighbourMax.frag.spv");
		reg->RegisterShader(static_cast<int32_t>(MotionBlurShaderID::RECONSTRUCTION),
			"motionblur/Reconstruction.vert", "motionblur/Reconstruction.frag",
			"spirv/motionblur/Reconstruction.vert.spv", "spirv/motionblur/Reconstruction.frag.spv");
		reg->RegisterShader(static_cast<int32_t>(MotionBlurShaderID::MOTION_BLUR),
			"motionblur/MotionBlur.vert", "motionblur/MotionBlur.frag",
			"spirv/motionblur/MotionBlur.vert.spv", "spirv/motionblur/MotionBlur.frag.spv");
	});

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
