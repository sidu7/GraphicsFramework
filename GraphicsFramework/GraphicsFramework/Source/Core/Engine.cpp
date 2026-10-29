#pragma once

// External
#include <iostream>
#include "glm/gtc/matrix_transform.hpp"
#include <rapidjson/rapidjson.h>

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
#include "ComponentFactory.h"
#include "Utils/JSONHelper.h"
#include "Profiling/ProfilerInternal.h"
#include "Profiling/Profiler.h"

void Engine::Start(const std::string& settingsFilePath)
{
	mSettingsFilePath = settingsFilePath;
	rapidjson::Document SettingsFile = JSONHelper::ParseFile(mSettingsFilePath);

	if (SettingsFile.HasParseError())
	{
		std::cout << "Could not parse settings file: " << mSettingsFilePath << std::endl;
		return;
	}

	mSettings.Parse(SettingsFile);

	Window::Instance()->Init(mSettings.mWindowSize.x, mSettings.mWindowSize.y);

	appIsRunning = true;

	// Init managers
	Inputs::Instance()->Init();
	Renderer::Instance()->Init();
	Time::Instance()->Init(120);
	ShapeManager::Instance()->Init();
	ImguiManager::Instance()->Init();
	ComponentFactory::Instance()->Init();

	// Initialize profiler with settings from Settings.json
	Profiler::Instance()->Init(mSettings.mProfilerSettings);

	pCamera = new Camera();
	pCamera->Init(glm::radians(45.0f), 0.1f, 4000.0f);

	mPause = false;
}

void Engine::Run()
{
	while (appIsRunning)
	{
		// ── Profiler: start new frame ──
		Profiler::Instance()->StartFrame();
		{
			GF_PROFILE_SCOPE("Engine::Run");
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
			if (pScene)
			{
				pScene->DebugDisplay();
				pScene->Update();
			}

			Profiler::Instance()->MarkRenderStart();
			Profiler::Instance()->MarkImguiStart();
			ImguiManager::Instance()->Update();
			Profiler::Instance()->MarkImguiEnd();

			Renderer::Instance()->SwapBuffers();
			Time::Instance()->FrameEnd();
			Profiler::Instance()->MarkRenderEnd();

			for (const NextFrameFunc& Func : EndFrameFuncs)
			{
				Func();
			}
			EndFrameFuncs.clear();
		}

		// ── Profiler: end frame ──
		Profiler::Instance()->EndFrame();
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
	ImguiManager::Instance()->Close();

	// The profiler owns Vulkan/OpenGL objects (query pools, GL query names), so
	// it must be shut down before the renderer destroys the device/context.
	Profiler::Instance()->Close();

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
	if (Root.HasMember("TransformBindingPoint"))
	{
		mTransformBindingPoint = Root["TransformBindingPoint"].GetInt();
	}
	if (Root.HasMember("MaterialBindingPoint"))
	{
		mMaterialBindingPoint = Root["MaterialBindingPoint"].GetInt();
	}

	// Profiler settings (optional — uses defaults if absent)
	if (Root.HasMember("Profiler"))
	{
		rapidjson::Value::Object Profiler = Root["Profiler"].GetObject();
		if (Profiler.HasMember("MaxConcurrentQueries"))
		{
			mProfilerSettings.maxConcurrentQueries = static_cast<uint32_t>(Profiler["MaxConcurrentQueries"].GetUint());
		}
		if (Profiler.HasMember("Enabled"))
		{
			mProfilerSettings.enabled = Profiler["Enabled"].GetBool();
		}
		if (Profiler.HasMember("CaptureDrawCalls"))
		{
			mProfilerSettings.captureDrawCalls = Profiler["CaptureDrawCalls"].GetBool();
		}
	}
}
