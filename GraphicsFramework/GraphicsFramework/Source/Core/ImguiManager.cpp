#include "ImguiManager.h"

#include "Core.h"
#include "Engine.h"
#include "Camera.h"
#include "Window.h"
#include "Imgui/imgui.h"
#include "Imgui/backends/imgui_impl_sdl2.h"
#if defined(RENDERER_OPENGL)
#include "Imgui/backends/imgui_impl_opengl3.h"
#include "RHI/OpenGL/WindowGL.h"
#elif defined(RENDERER_VULKAN)
#include "RHI/Vulkan/DeviceVK.h"
#include "RHI/Vulkan/FrameBufferVK.h"
#include "RHI/Vulkan/Internal/HelperVK.h"
#include "RHI/Vulkan/Internal/CommandBufferVK.h"
#include "RHI/Vulkan/Internal/CommandQueueVK.h"
#include "RHI/Vulkan/Internal/RenderPassVK.h"
	#include "Imgui/backends/imgui_impl_vulkan.h"
#elif defined(RENDERER_DX12)
#endif

void ImguiManager::Init()
{
#if !NO_IMGUI
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui::StyleColorsDark();

	// Set up renderer bindings	
#if defined(RENDERER_OPENGL)
	rhi::gl::WindowGL* WindowOpenGL = static_cast<rhi::gl::WindowGL*>(Window::Instance());
	ImGui_ImplSDL2_InitForOpenGL(WindowOpenGL->GetWindow(), WindowOpenGL->GetContext());
	ImGui_ImplOpenGL3_Init("#version 430");
#elif defined(RENDERER_VULKAN)
	ImGui_ImplSDL2_InitForVulkan(Window::Instance()->GetWindow());

	rhi::vk::Helper VkHelper;
	VkHelper.InitImGui();
#elif defined(RENDERER_DX12)
#endif
#endif
}

void ImguiManager::StartFrame()
{
#if !NO_IMGUI
#if defined(RENDERER_OPENGL)
	ImGui_ImplOpenGL3_NewFrame();
#elif defined(RENDERER_VULKAN)
	ImGui_ImplVulkan_NewFrame();
#elif defined(RENDERER_DX12)
#endif
	ImGui_ImplSDL2_NewFrame();
	ImGui::NewFrame();
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
	ImGui::End();
	Render();
#endif
}

void ImguiManager::Close()
{
#if !NO_IMGUI
#if defined(RENDERER_OPENGL)
	ImGui_ImplOpenGL3_Shutdown();
#elif defined(RENDERER_VULKAN)
	ImGui_ImplVulkan_Shutdown();
#elif defined(RENDERER_DX12)
#endif
	ImGui_ImplSDL2_Shutdown();
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
#if defined(RENDERER_OPENGL)
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
#elif defined(RENDERER_VULKAN)
	rhi::vk::Device* VkRenderer = static_cast<rhi::vk::Device*>(Renderer::Instance());
	if (VkRenderer)
	{
		const rhi::vk::FrameBufferVK* VkFrameBuffer = static_cast<const rhi::vk::FrameBufferVK*>(VkRenderer->GetBackBuffer());
		VkRenderer->GetImGuiRenderPass()->BeginPass(VkRenderer->GetCommandBuffer(), VkFrameBuffer);
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), VkRenderer->GetCommandBuffer()->mCommandBuffer);
		VkRenderer->GetImGuiRenderPass()->EndPass(VkRenderer->GetCommandBuffer());
	}
#elif defined(RENDERER_DX12)
#endif
}