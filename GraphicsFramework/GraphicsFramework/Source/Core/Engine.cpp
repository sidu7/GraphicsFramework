#pragma once

// External
#include <iostream>
#include "glm/gtc/matrix_transform.hpp"
#ifdef RENDERER_VULKAN
#include <SDL_vulkan.h>
#endif

// Internal
#include "Window.h"
#include "Inputs.h"
#include "Rendering/Renderer.h"
#include "Time.h"
#include "Engine.h"
#include "Camera.h"
#include "Light.h"
#include "ShapeManager.h"
#include "Scene.h"
#include "ImguiManager.h"
#include "ObjectManager.h"
#include "ComponentManager.h"


void Engine::Start(Scene* scene,int width, int height)
{
	Window::Instance()->Init(width, height);

	appIsRunning = true;

	//Init Managers
	Inputs::Instance()->Init();
	Renderer::Instance()->Init();
	Time::Instance()->Init(60);
	ShapeManager::Instance()->Init();
	ImguiManager::Instance()->Init();
	ComponentManager::Instance()->Init();

	pCamera = new Camera();
	pCamera->Init(glm::radians(45.0f), 0.1f, 4000.0f);

	mPause = false;

	pScene = scene;
	pScene->Init();
}

void Engine::Run()
{
	while (appIsRunning)
	{
		Time::Instance()->FrameStart();	
		ImguiManager::Instance()->StartFrame();

		Renderer::Instance()->Clear();
		Inputs::Instance()->Update();
		pCamera->Update();

		pScene->DebugDisplay();
		pScene->Update();
	
		ImguiManager::Instance()->Update();
		Renderer::Instance()->SwapBuffers();
		Time::Instance()->FrameEnd();
	}
}

void Engine::Stop()
{
	ImguiManager::Instance()->Close();
	ObjectManager::Instance()->Close();
	ShapeManager::Instance()->Close();
	ComponentManager::Instance()->Close();
	std::cout << "Window closed" << std::endl;
	Window::Instance()->Close();
	delete pCamera;
}

Camera* Engine::GetCamera()
{
	return pCamera;
}
