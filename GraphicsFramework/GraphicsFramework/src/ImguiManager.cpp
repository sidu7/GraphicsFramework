#include "ImguiManager.h"
#include "Engine.h"

extern Engine* engine;

void ImguiManager::Init()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui::StyleColorsDark();

	// Set up renderer bindings
	ImGui_ImplSDL2_InitForOpenGL(engine->pWindow, engine->glContext);
	ImGui_ImplOpenGL3_Init("#version 430");
}

void ImguiManager::StartFrame()
{
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplSDL2_NewFrame(engine->pWindow);
	ImGui::NewFrame();
}

void ImguiManager::Update()
{
	ImGui::Begin("FPS");
	ImGui::Text("FPS: %.2f FPS", ImGui::GetIO().Framerate);
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