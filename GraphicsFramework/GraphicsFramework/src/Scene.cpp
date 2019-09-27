#include "Scene.h"
#include "Engine.h"
#include "Inputs.h"
#include "ShapeManager.h"
#include "Camera.h"
#include "glm/gtc/matrix_transform.hpp"

extern Engine* engine;

void Scene::Init()
{
	shader = new Shader("src/shaders/Drawing.vert", "src/shaders/Drawing.frag");

	lighting = new Shader("src/shaders/Lighting.vert", "src/shaders/Lighting.frag");

	ambient = new Shader("src/shaders/Ambient.vert", "src/shaders/Ambient.frag");

	demoModel.LoadModel("res/gh_sample_animation.fbx");

	light = new Light();
	light->position = glm::vec3(0.0f, 20.0f, 10.0f);


	lighting->Bind();
	lighting->SetUniform3f("Light", 3.2f, 3.2f, 3.2f);

	ambient->Bind();
	ambient->SetUniform3f("Ambient", 0.3f, 0.3f, 0.3f);


	G_Buffer = new FrameBuffer(engine->scrWidth, engine->scrHeight, 4);

	gBuffershow = 0;
}

void Scene::Draw()
{
	light->HandleInputs();



	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);

	G_Buffer->Bind();
	shader->Bind();
	shader->SetUniformMat4f("view", engine->pCamera->mView);
	shader->SetUniformMat4f("projection", engine->pCamera->mProjection);


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
	model = glm::scale(model, glm::vec3(30.0f));
	shader->SetUniformMat4f("model", model);
	shader->SetUniformMat4f("normaltr", glm::inverse(model));
	shader->SetUniform3f("diffuse", 0.5f, 1.0f, 0.0f);
	shader->SetUniform3f("specular", 1.0f, 1.0f, 1.0f);
	shader->SetUniform1f("shininess", 10.0f);
	shape = ShapeManager::Instance().mShapes[Shapes::QUAD];
	Renderer::Instance().Draw(*shape.first, *shape.second, *shader);

	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
	model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0, 0.0, 0.0));
	model = glm::scale(model, glm::vec3(1.0f));
	shader->SetUniformMat4f("model", model);
	shader->SetUniformMat4f("normaltr", glm::inverse(model));
	shader->SetUniform3f("diffuse", 0.5f, 0.0f, 1.0f);
	shader->SetUniform3f("specular", 0.0f, 1.0f, 0.0f);
	shader->SetUniform1f("shininess", 1.0f);
	shape = ShapeManager::Instance().mShapes[Shapes::TEAPOT];
	Renderer::Instance().Draw(*shape.first, *shape.second, *shader);

	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(10.0f, 0.0f, 0.0f));
	model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0, 0.0, 0.0));
	model = glm::scale(model, glm::vec3(1.0f));
	shader->SetUniformMat4f("model", model);
	shader->SetUniformMat4f("normaltr", glm::inverse(model));
	shader->SetUniform3f("diffuse", 1.0f, 0.0f, 0.0f);
	shader->SetUniform3f("specular", 0.0f, 0.0f, 1.0f);
	shader->SetUniform1f("shininess", 1.0f);
	shape = ShapeManager::Instance().mShapes[Shapes::TEAPOT];
	Renderer::Instance().Draw(*shape.first, *shape.second, *shader);

	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(-10.0f, 0.0f, 0.0f));
	model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0, 0.0, 0.0));
	model = glm::scale(model, glm::vec3(1.0f));
	shader->SetUniformMat4f("model", model);
	shader->SetUniformMat4f("normaltr", glm::inverse(model));
	shader->SetUniform3f("diffuse", 1.0f, 0.5f, 0.0f);
	shader->SetUniform3f("specular", 0.5f, 0.5f, 0.5f);
	shader->SetUniform1f("shininess", 1.0f);
	shape = ShapeManager::Instance().mShapes[Shapes::TEAPOT];
	Renderer::Instance().Draw(*shape.first, *shape.second, *shader);

	shader->Unbind();
	G_Buffer->Unbind();

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);

	ambient->Bind();
	G_Buffer->TexBind(2, 3);
	ambient->SetUniform1i("diffusetex", 3);
	Renderer::Instance().DrawQuad();
	ambient->Unbind();

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
	//lighting->SetUniform3f("lightPos", light->position.x, light->position.y, light->position.z);
	lighting->SetUniform3f("lightPos", engine->pCamera->mCameraPos.x, engine->pCamera->mCameraPos.y, engine->pCamera->mCameraPos.z);
	lighting->SetUniformMat4f("inverseview", glm::inverse(engine->pCamera->mView));	
	if (Inputs::Instance().IsTriggered(SDL_SCANCODE_R))
	{
		gBuffershow = 0;
	}
	if (Inputs::Instance().IsTriggered(SDL_SCANCODE_1))
	{
		gBuffershow = 1;
	}
	if (Inputs::Instance().IsTriggered(SDL_SCANCODE_2))
	{
		gBuffershow = 2;
	}
	if (Inputs::Instance().IsTriggered(SDL_SCANCODE_3))
	{
		gBuffershow = 3;
	}
	if (Inputs::Instance().IsTriggered(SDL_SCANCODE_4))
	{
		gBuffershow = 4;
	}
	lighting->SetUniform1i("GBufferShow", gBuffershow);
	Renderer::Instance().DrawQuad();
	lighting->Unbind();
}
