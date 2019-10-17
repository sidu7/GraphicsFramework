#include "Scene.h"
#include "Engine.h"
#include "Inputs.h"
#include "ShapeManager.h"
#include "Camera.h"
#include "glm/gtc/matrix_transform.hpp"
#include "Time.h"
#include "ImguiManager.h"
#include <time.h>

extern Engine* engine;

float lp[3];

void Scene::Init()
{
	shader = new Shader("src/shaders/Drawing.vert", "src/shaders/Drawing.frag");

	lighting = new Shader("src/shaders/Lighting.vert", "src/shaders/Lighting.frag");

	ambient = new Shader("src/shaders/Ambient.vert", "src/shaders/Ambient.frag");

	shadow = new Shader("src/shaders/Shadow.vert","src/shaders/Shadow.frag");

	locallight = new Shader("src/shaders/LocalLight.vert", "src/shaders/LocalLight.frag");

	light = new Light();
	light->position = glm::vec3(100.0f, 200.0f, 10.0f);
	lp[0] = 100.0f;
	lp[1] = 200.0f;
	lp[2] = 10.0f;
	light->distance = 200.0f;

	engine->pCamera->pitch = -20.0f;
	engine->pCamera->mCameraPos = glm::vec3(0.0f, 40.0f, 100.0f);
	engine->pCamera->CalculateFront();

	lighting->Bind();
	lighting->SetUniform3f("Light", 1.0f, 1.0f, 1.0f);

	ambient->Bind();
	ambient->SetUniform3f("Ambient", 0.1f, 0.1f, 0.1f);

	G_Buffer = new FrameBuffer(engine->scrWidth, engine->scrHeight, 4);
	ShadowMap = new FrameBuffer(1024, 1024);

	gBuffershow = 0;

	srand(time(NULL));

	for (unsigned int i = 0; i < 40; ++i)
	{
		for (unsigned int j = 0; j < 40; ++j)
		{
			lightColors[i][j] = glm::vec3((rand() % 32) / 10.0f, (rand() % 32) / 10.0f, (rand() % 32) / 10.0f);
		}
	}
}

bool lighton = true;

void Scene::Draw()
{
	light->HandleInputs();

	ImGui::Begin("G-Buffer");
	ImGui::Checkbox("Lighting", &lighton);
	bool selected = true;
	if (ImGui::BeginCombo("", "Select G-Buffer"))
	{
		lighton = true;
		if (ImGui::Selectable("Lighting", selected))
			gBuffershow = 0;
		if (ImGui::Selectable("Normals", selected))
			gBuffershow = 1;
		if (ImGui::Selectable("WorldPos", selected))
			gBuffershow = 2;
		if (ImGui::Selectable("Diffuse", selected))
			gBuffershow = 3;
		if (ImGui::Selectable("Specular", selected))
			gBuffershow = 4;
		ImGui::EndCombo();
	}
	if (ImGui::DragFloat3("Light Position", lp, 2.0f, -300.0f, 300.0f))
	{
		light->position = glm::vec3(lp[0], lp[1], lp[2]);
	}
	ImGui::End();
	
	//G-Buffer pass
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
	G_Buffer->Bind();
	shader->Bind();
	shader->SetUniformMat4f("view", engine->pCamera->mView);
	shader->SetUniformMat4f("projection", engine->pCamera->mProjection);

	ObjectsDraw(shader);

	shader->Unbind();
	G_Buffer->Unbind();

	//ShadowMap pass
	ShadowMap->Bind();
	shadow->Bind();

	glm::mat4 LightLookAt, shadowMatrix, LightProj;
	LightLookAt = glm::lookAt(light->position, glm::vec3(0.0f,0.0f,0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	LightProj = glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 2000.0f);

	shadow->SetUniformMat4f("projection", LightProj);
	shadow->SetUniformMat4f("view", LightLookAt);

	shadowMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.5f)) * glm::scale(glm::mat4(1.0f), glm::vec3(0.5f)) * LightProj * LightLookAt;

	glEnable(GL_CULL_FACE);
	
	ObjectsDraw(shadow);

	glDisable(GL_CULL_FACE);

	shadow->Unbind();
	ShadowMap->Unbind();

	//Ambient light pass
	glViewport(0, 0, engine->scrWidth, engine->scrHeight);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);

	ambient->Bind();
	G_Buffer->TexBind(2, 3);
	ambient->SetUniform1i("diffusetex", 3);
	Renderer::Instance().DrawQuad();
	ambient->Unbind();

	if (gBuffershow == 0)
	{
		glEnable(GL_BLEND);
	}

	if (lighton)
	{
		lighting->Bind();
		G_Buffer->TexBind(0, 1);
		lighting->SetUniform1i("normaltex", 1);
		G_Buffer->TexBind(1, 2);
		lighting->SetUniform1i("worldpostex", 2);
		G_Buffer->TexBind(2, 3);
		lighting->SetUniform1i("diffusetex", 3);
		G_Buffer->TexBind(3, 4);
		lighting->SetUniform1i("specularalpha", 4);
		ShadowMap->TexBind(0, 5);
		lighting->SetUniform1i("shadowmap", 5);
		lighting->SetUniformMat4f("shadowmat", shadowMatrix);
		lighting->SetUniform3f("lightPos", light->position.x, light->position.y, light->position.z);
		lighting->SetUniformMat4f("inverseview", glm::inverse(engine->pCamera->mView));
		lighting->SetUniform1i("GBufferShow", gBuffershow);
		Renderer::Instance().DrawQuad();
		lighting->Unbind();
	}

	if (gBuffershow == 0)
	{
		glEnable(GL_CULL_FACE);
		glCullFace(GL_FRONT);

		locallight->Bind();
		locallight->SetUniformMat4f("view", engine->pCamera->mView);
		locallight->SetUniformMat4f("projection", engine->pCamera->mProjection);
		G_Buffer->TexBind(0, 1);
		locallight->SetUniform1i("normaltex", 1);
		G_Buffer->TexBind(1, 2);
		locallight->SetUniform1i("worldpostex", 2);
		G_Buffer->TexBind(2, 3);
		locallight->SetUniform1i("diffusetex", 3);
		G_Buffer->TexBind(3, 4);
		locallight->SetUniform1i("specularalpha", 4);
		locallight->SetUniformMat4f("inverseview", glm::inverse(engine->pCamera->mView));
		float lightRadius = 4.0f;
		locallight->SetUniform1f("lightRadius", lightRadius);
		for (unsigned int i = 0; i < 40; ++i)
		{
			for (unsigned int j = 0; j < 40; ++j)
			{
				glm::mat4 model = glm::mat4(1.0f);
				model = glm::translate(model, glm::vec3(i * lightRadius * 2 - 40.0f * lightRadius, lightRadius / 2 , j * lightRadius * 2 - 40.0f * lightRadius));
				model = glm::scale(model, glm::vec3(lightRadius));
				locallight->SetUniform3f("lightPos", i * lightRadius * 2 - 40.0f * lightRadius, lightRadius / 2, j * lightRadius * 2 - 40.0f * lightRadius);
				locallight->SetUniformMat4f("model", model);
				locallight->SetUniform3f("Light",lightColors[i][j].x, lightColors[i][j].y, lightColors[i][j].z);
				std::pair<VertexArray*, ElementArrayBuffer*> shape = ShapeManager::Instance().mShapes[Shapes::SPHERE];
				Renderer::Instance().Draw(*shape.first, *shape.second, *locallight);
			}
		}

		glDisable(GL_CULL_FACE);
	}
}
float t = 0;
void Scene::ObjectsDraw(Shader* shader)
{	

	glm::mat4 model = glm::mat4(1.0f);
	std::pair<VertexArray*, ElementArrayBuffer*> shape;
	/*model = glm::translate(model, light->position);
	shader->SetUniformMat4f("model", model);
	shader->SetUniformMat4f("normaltr", glm::inverse(model));
	shader->SetUniform3f("diffuse", 0.0f, 1.0f, 0.0f);
	std::pair<VertexArray*, ElementArrayBuffer*> shape = ShapeManager::Instance().mShapes[Shapes::SPHERE];
	Renderer::Instance().Draw(*shape.first, *shape.second, *shader);*/
	   
	if (glIsEnabled(GL_CULL_FACE))
	{
		glCullFace(GL_BACK);
	}

	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
	model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0, 0.0, 0.0));
	model = glm::scale(model, glm::vec3(1500.0f));
	shader->SetUniformMat4f("model", model);
	shader->SetUniformMat4f("normaltr", glm::inverse(model));
	shader->SetUniform3f("diffuse", 1.0f, 1.0f, 1.0f);
	shader->SetUniform3f("specular", 0.2f, 0.2f, 0.2f);
	shader->SetUniform1f("shininess", 1.0f);
	shape = ShapeManager::Instance().mShapes[Shapes::QUAD];
	Renderer::Instance().Draw(*shape.first, *shape.second, *shader);

	if (glIsEnabled(GL_CULL_FACE))
	{
		glCullFace(GL_FRONT);
	}

	angle += Time::Instance().deltaTime * 50.0f;
	t += Time::Instance().deltaTime;
	trans1 = glm::vec3(40.0f * glm::cos(t * 0.09), 2.0f, 40.0f * glm::sin(t * 0.09));
	trans2 = glm::vec3(60.0f * glm::cos(t * 0.08), 4.0f, 60.0f * glm::sin(t*0.08));
	trans3 = glm::vec3(20.0f*glm::cos(t * 0.1), 0.0f, 20.0f * glm::sin(t * 0.1));
	
	model = glm::mat4(1.0f);
	model = glm::translate(model, trans1);
	model = glm::rotate(model, glm::radians(angle), glm::vec3(0.0, 1.0, 0.0));
	model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0, 0.0, 0.0));
	model = glm::scale(model, glm::vec3(3.0f));
	shader->SetUniformMat4f("model", model);
	shader->SetUniformMat4f("normaltr", glm::inverse(model));
	shader->SetUniform3f("diffuse", 0.0f, 0.0f, 1.0f);
	shader->SetUniform3f("specular", 1.0f, 1.0f, 0.0f);
	shader->SetUniform1f("shininess", 100.0f);
	shape = ShapeManager::Instance().mShapes[Shapes::TEAPOT];
	Renderer::Instance().Draw(*shape.first, *shape.second, *shader);

	model = glm::mat4(1.0f);
	model = glm::translate(model, trans2);
	model = glm::rotate(model, glm::radians(angle), glm::vec3(0.0, 1.0, 0.0));
	model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0, 0.0, 0.0));
	model = glm::scale(model, glm::vec3(3.0f));
	shader->SetUniformMat4f("model", model);
	shader->SetUniformMat4f("normaltr", glm::inverse(model));
	shader->SetUniform3f("diffuse", 1.0f, 0.5f, 0.0f);
	shader->SetUniform3f("specular", 0.0f, 0.5f, 0.5f);
	shader->SetUniform1f("shininess", 50.0f);
	shape = ShapeManager::Instance().mShapes[Shapes::TEAPOT];
	Renderer::Instance().Draw(*shape.first, *shape.second, *shader);

	model = glm::mat4(1.0f);
	model = glm::translate(model, trans3);
	model = glm::rotate(model, glm::radians(angle), glm::vec3(0.0, 1.0, 0.0));
	model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0, 0.0, 0.0));
	model = glm::scale(model, glm::vec3(3.0f));
	shader->SetUniformMat4f("model", model);
	shader->SetUniformMat4f("normaltr", glm::inverse(model));
	shader->SetUniform3f("diffuse", 1.0f, 0.0f, 0.0f);
	shader->SetUniform3f("specular", 0.0f, 0.0f, 1.0f);
	shader->SetUniform1f("shininess", 10.0f);
	shape = ShapeManager::Instance().mShapes[Shapes::TEAPOT];
	Renderer::Instance().Draw(*shape.first, *shape.second, *shader);
}