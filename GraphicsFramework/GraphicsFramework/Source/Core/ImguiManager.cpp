#include "ImguiManager.h"

#include "Core.h"
#include "Engine.h"
#include "Camera.h"
#include "Window.h"
#include "Imgui/imgui.h"
#include "Imgui/backends/imgui_impl_sdl2.h"
#include "Rendering/Renderer.h"

#ifdef GF_PROFILING
#include "Profiling/Profiler.h"
#endif

void ImguiManager::Init()
{
#if !NO_IMGUI
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui::StyleColorsDark();

	// Set up backend ImGui bindings
	Renderer::Instance()->InitImGui();
#endif
}

void ImguiManager::StartFrame()
{
#if !NO_IMGUI
	ImGui_ImplSDL2_NewFrame();
	ImGui::NewFrame();
	Renderer::Instance()->NewFrame();
#endif
}

void ImguiManager::Update()
{
#if !NO_IMGUI
	Camera* Cam = Engine::Instance()->GetCamera();
	ImGui::Begin("FPS");
	ImGui::Text("FPS: %.2f FPS", ImGui::GetIO().Framerate);
	ImGui::Text("Camera Position: %.1f, %.1f, %.1f", Cam->mCameraPos.x, Cam->mCameraPos.y, Cam->mCameraPos.z);
	ImGui::Text("Camera Pitch: %.1f Yaw %.1f", Cam->pitch, Cam->yaw);

#ifdef GF_PROFILING
	if (ImGui::Button("Toggle Profiler"))
	{
		Profiler::Instance()->SetOverlayVisible(!Profiler::Instance()->IsOverlayVisible());
	}
#endif

	ImGui::End();
#ifdef GF_PROFILING
	Profiler::Instance()->RenderOverlay();
#endif
	Render();
#endif
}

void ImguiManager::Close()
{
#if !NO_IMGUI
	Renderer::Instance()->ShutdownImGui();
	ImGui::DestroyContext();
#endif
}

bool ImguiManager::IsAnyWindowFocused() const
{
#if !NO_IMGUI
	return ImGui::IsWindowFocused(ImGuiFocusedFlags_AnyWindow);
#else
	return false;
#endif
}

void ImguiManager::ProcessWindowEvent(const SDL_Event* windowEvent)
{
	ImGui_ImplSDL2_ProcessEvent(windowEvent);
}

void ImguiManager::Render()
{
	ImGui::Render(); 
	Renderer::Instance()->Render();
}