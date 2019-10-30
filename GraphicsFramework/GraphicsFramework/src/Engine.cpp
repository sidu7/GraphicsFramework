#include "Inputs.h"
#include "Time.h"
#include "Engine.h"
#include "../opengl/Renderer.h"
#include "../opengl/VertexArray.h"
#include "../opengl/VertexBuffer.h"
#include "../opengl/ElementArrayBuffer.h"
#include "../opengl/Shader.h"
#include "Camera.h"
#include <iostream>
#include "../opengl/Model.h"
#include "glm/gtc/matrix_transform.hpp"
#include "Camera.h"
#include "Light.h"
#include "ShapeManager.h"
#include "../opengl/FrameBuffer.h"
#include "Scene.h"
#include "ImguiManager.h"
#include "AnimationScene.h"


void Engine::Start()
{
	int error = 0;

	//Initialize SDL
	if ((error = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER)) < 0)//Init for video and joystick
	{
		printf("Couldn't initialize SDL, error %i\n", error);
		return;
	}
	//printf("%i joysticks were found.\n\n", SDL_NumJoysticks());//NUmber of joysticks
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	// Enable AntiAliasiing (Don't know what does this do)
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

	scrWidth = 1280;
	scrHeight = 720;

	pWindow = SDL_CreateWindow("Graphics Framework",
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		scrWidth,
		scrHeight,
		SDL_WINDOW_OPENGL);


	//Check if window was made successfully
	if (pWindow == NULL)
	{
		printf("Could not create window: %s\n", SDL_GetError());
		return;
	}


	glContext = SDL_GL_CreateContext(pWindow);
	if (glContext == NULL)
	{
		printf("OpenGL context could not be created. SDL Error: %s\n", SDL_GetError());
	}

	appIsRunning = true;

	//Init Managers
	Inputs::Instance().Init();
	Renderer::Instance().Init(pWindow);
	Time::Instance().Init(60);
	ShapeManager::Instance().Init();
	ImguiManager::Instance().Init();

	pCamera = new Camera();
	pCamera->Init(glm::radians(45.0f), 0.1f, 2000.0f);

	//CS562
	//Scene::Instance().Init();

	//CS560
	AnimationScene::Instance().Init();
}

void Engine::Run()
{
	while (appIsRunning)
	{
		Time::Instance().FrameStart();	
		ImguiManager::Instance().StartFrame();

		Renderer::Instance().Clear();
		Inputs::Instance().Update();
		pCamera->Update();
		
		//CS562
		//Scene::Instance().Draw();

		//CS560
		AnimationScene::Instance().Draw();

		ImguiManager::Instance().Update();
		Renderer::Instance().SwapBuffers();
		Time::Instance().FrameEnd();
	}
}

void Engine::Stop()
{
	ImguiManager::Instance().Close();
	std::cout << "Window closed" << std::endl;
	SDL_DestroyWindow(pWindow);
	SDL_Quit();
}

glm::vec2 Engine::GetWindowSize()
{
	int x, y;
	SDL_GetWindowSize(pWindow, &x, &y);
	return glm::vec2(x, y);
}
