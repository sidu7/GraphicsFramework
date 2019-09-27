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

Shader* shader;
Shader* lighting;
Model demoModel;

Light* light;
FrameBuffer* G_Buffer;

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


	SDL_GLContext glContext = SDL_GL_CreateContext(pWindow);
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

	
	shader = new Shader("src/shaders/Drawing.vert", "src/shaders/Drawing.frag");

	lighting = new Shader("src/shaders/Lighting.vert", "src/shaders/Lighting.frag");

	demoModel.LoadModel("res/gh_sample_animation.fbx");
	


	pCamera = new Camera();
	pCamera->Init(glm::radians(45.0f), 0.1f, 100.0f);

	light = new Light();
	light->position = glm::vec3(0.0f, 0.0f, 10.0f);

	shader->Bind();
	shader->SetUniform3f("diffuse", 1.0f, 0.5f, 0.0f);
	shader->SetUniform3f("specular", 1.0f, 1.0f, 1.0f);
	shader->SetUniform1f("shininess", 10.0f);

	lighting->Bind();
	lighting->SetUniform3f("Light", 3.2f, 3.2f, 3.2f);
	lighting->SetUniform3f("Ambient", 0.2f, 0.2f, 0.2f);

	G_Buffer = new FrameBuffer(scrWidth,scrHeight,4);
}

void Engine::Run()
{
	while (appIsRunning)
	{
		Time::Instance().FrameStart();		
		Renderer::Instance().Clear();
		Inputs::Instance().Update();
		pCamera->Update();
		light->HandleInputs();

		G_Buffer->Bind();
		shader->Bind();
		shader->SetUniformMat4f("view", pCamera->mView);
		//shader->SetUniformMat4f("inverseview", glm::inverse(pCamera->mView));
		shader->SetUniformMat4f("projection", pCamera->mProjection);

		
		//Renderer::Instance().DrawQuad();
		//demoModel.Draw(shader);
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, light->position);
		shader->SetUniformMat4f("model", model);
		shader->SetUniformMat4f("normaltr", glm::inverse(model));
		std::pair<VertexArray*, ElementArrayBuffer*> shape = ShapeManager::Instance().mShapes[Shapes::SPHERE];
		Renderer::Instance().Draw(*shape.first, *shape.second, *shader);

		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0, 0.0, 0.0));
		model = glm::scale(model, glm::vec3(1.0f));
		shader->SetUniformMat4f("model", model);
		shader->SetUniformMat4f("normaltr", glm::inverse(model));
		shape = ShapeManager::Instance().mShapes[Shapes::TEAPOT];
		Renderer::Instance().Draw(*shape.first, *shape.second, *shader);

		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(10.0f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0, 0.0, 0.0));
		model = glm::scale(model, glm::vec3(1.0f));
		shader->SetUniformMat4f("model", model);
		shader->SetUniformMat4f("normaltr", glm::inverse(model));
		shape = ShapeManager::Instance().mShapes[Shapes::TEAPOT];
		Renderer::Instance().Draw(*shape.first, *shape.second, *shader);

		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(-10.0f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0, 0.0, 0.0));
		model = glm::scale(model, glm::vec3(1.0f));
		shader->SetUniformMat4f("model", model);
		shader->SetUniformMat4f("normaltr", glm::inverse(model));
		shape = ShapeManager::Instance().mShapes[Shapes::TEAPOT];
		Renderer::Instance().Draw(*shape.first, *shape.second, *shader);

		shader->Unbind();
		G_Buffer->Unbind();

		glDisable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);

		lighting->Bind();
		G_Buffer->TexBind(0, 1);
		lighting->SetUniform1i("normaltex", 1);
		G_Buffer->TexBind(1, 2);
		lighting->SetUniform1i("worldpostex", 2);
		G_Buffer->TexBind(2, 3);
		lighting->SetUniform1i("diffusetex", 3);
		G_Buffer->TexBind(3, 4);
		lighting->SetUniform1i("specularalpha", 4);
		lighting->SetUniform3f("lightPos", light->position.x, light->position.y, light->position.z);
		glm::vec4 eyePos = glm::inverse(pCamera->mView) * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
		lighting->SetUniform3f("eyePos", eyePos.x, eyePos.y, eyePos.z);
		Renderer::Instance().DrawQuad();
		lighting->Unbind();
		
		glEnable(GL_DEPTH_TEST);
		glDisable(GL_BLEND);
		//glBlendFunc(GL_ONE, GL_ONE);

		Renderer::Instance().SwapBuffers();
		Time::Instance().FrameEnd();
	}
}

void Engine::Stop()
{
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
