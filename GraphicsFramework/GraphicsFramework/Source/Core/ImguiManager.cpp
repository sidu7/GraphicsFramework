#include "ImguiManager.h"

#include "Core.h"
#include "Engine.h"
#include "Camera.h"
#include "Window.h"
#include "Imgui/imgui.h"
#include "Imgui/imgui_impl_opengl3.h"
#include "Imgui/imgui_impl_sdl.h"

void ImguiManager::Init()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui::StyleColorsDark();

	// Set up renderer bindings	
	ImGui_ImplSDL2_InitForOpenGL(Window::Instance()->GetWindow(), Window::Instance()->GetContext());
	ImGui_ImplOpenGL3_Init("#version 430");
}

void ImguiManager::StartFrame()
{
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplSDL2_NewFrame(Window::Instance()->GetWindow());
	ImGui::NewFrame();
}

void ImguiManager::Update()
{
	Camera* Cam = Engine::Instance()->GetCamera();
	ImGui::Begin("FPS");
	ImGui::Text("FPS: %.2f FPS", ImGui::GetIO().Framerate);
	ImGui::Text("Camera Position: %.1f, %.1f, %.1f", Cam->mCameraPos.x, Cam->mCameraPos.y, Cam->mCameraPos.z);
	ImGui::Text("Camera Pitch: %.1f Yaw %.1f", Cam->pitch, Cam->yaw);
	ImGui::End();
	Render();
}

void ImguiManager::Close()
{
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();
}

void ImguiManager::Render()
{
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}