#pragma once

// External
#include <iostream>
#include "glm/gtc/matrix_transform.hpp"
#ifdef RENDERER_VULKAN
#include <SDL_vulkan.h>
#endif
#include <rapidjson/rapidjson.h>

// Internal
#include "Window.h"
#include "Inputs.h"
#include "Rendering/Renderer.h"
#include "Rendering/RenderingFactory.h"
#include "Time.h"
#include "Engine.h"
#include "Camera.h"
#include "Light.h"
#include "ShapeManager.h"
#include "Scene.h"
#include "ImguiManager.h"
#include "ObjectManager.h"
#include "ComponentManager.h"
#include "Rendering/ShaderManager.h"
#include "Utils/JSONHelper.h"

void Engine::Start(const std::string& settingsFilePath)
{
	rapidjson::Document SettingsFile = JSONHelper::ParseFile(settingsFilePath);

	if (SettingsFile.HasParseError())
	{
		std::cout << "Could not parse settings file: " << settingsFilePath << std::endl;
		return;
	}

	mSettings.Parse(SettingsFile);

	Window::Instance()->Init(mSettings.mWindowSize.x, mSettings.mWindowSize.y);

	appIsRunning = true;

	//Init Managers
	Inputs::Instance()->Init();
	Renderer::Instance()->Init();
	RenderingFactory::Instance()->Init();
	Time::Instance()->Init(60);
	ShapeManager::Instance()->Init();
	ImguiManager::Instance()->Init();
	ComponentManager::Instance()->Init();
	ShaderManager::Instance()->Init(mSettings.mShadersListPath);

	pCamera = new Camera();
	pCamera->Init(glm::radians(45.0f), 0.1f, 4000.0f);

	mPause = false;
}

void Engine::Run()
{
	while (appIsRunning)
	{
		for (const NextFrameFunc& Func : StartFrameFuncs)
		{
			Func();
		}
		StartFrameFuncs.clear();

		Time::Instance()->FrameStart();	
		ImguiManager::Instance()->StartFrame();

		Renderer::Instance()->StartFrame();
		Inputs::Instance()->Update();
		pCamera->Update();

#if !NO_IMGUI
		// Call Custom debug functions
		for (CustomDebugFunc& Func : CustomDebugFunctions)
		{
			Func();
		}
#endif

		if (pScene)
		{
			pScene->DebugDisplay();
			pScene->Update();
		}
	
		ImguiManager::Instance()->Update();
		Renderer::Instance()->SwapBuffers();
		Time::Instance()->FrameEnd();

		for (const NextFrameFunc& Func : EndFrameFuncs)
		{
			Func();
		}
		EndFrameFuncs.clear();
	}

	Renderer::Instance()->WaitFrameFinish();
}

void Engine::Stop()
{
	if (pScene)
	{
		pScene->Close();
	}
	ObjectManager::Instance()->Close();
	ShapeManager::Instance()->Close();
	ComponentManager::Instance()->Close();
	ImguiManager::Instance()->Close();
	RenderingFactory::Instance()->Close();
	Renderer::Instance()->Close();
	std::cout << "Window closed" << std::endl;
	Window::Instance()->Close();
	delete pCamera;
}

void Engine::AddCustomDebugFunction(CustomDebugFunc DebugFunction)
{
	CustomDebugFunctions.push_back(DebugFunction);
}

void Engine::ExecuteOnNextFrameStart(NextFrameFunc NextFrameFunction)
{
	StartFrameFuncs.push_back(NextFrameFunction);
}

void Engine::ExecuteOnNextFrameEnd(NextFrameFunc NextFrameFunction)
{
	EndFrameFuncs.push_back(NextFrameFunction);
}

Camera* Engine::GetCamera()
{
	return pCamera;
}

void Engine::ClearScene()
{
	if (pScene)
	{
		Renderer::Instance()->WaitFrameFinish();

		pScene->Close();
		delete pScene;
		pScene = nullptr;
	}
}

void Settings::Parse(rapidjson::Document& settingsFile)
{
	rapidjson::Value::Object Root = settingsFile.GetObject();
	if (Root.HasMember("WindowSize"))
	{
		mWindowSize = JSONHelper::GetVec2F(Root["WindowSize"].GetArray());
	}
	if (Root.HasMember("ShadersListFile"))
	{
		mShadersListPath = Root["ShadersListFile"].GetString();
	}
	if (Root.HasMember("TransformBindingPoint"))
	{
		mTransformBindingPoint = Root["TransformBindingPoint"].GetInt();
	}
	if (Root.HasMember("MaterialBindingPoint"))
	{
		mTransformBindingPoint = Root["MaterialBindingPoint"].GetInt();
	}
}
