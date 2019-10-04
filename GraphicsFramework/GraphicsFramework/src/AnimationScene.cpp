#include "AnimationScene.h"
#include "Engine.h"
#include "Camera.h"
#include "glm/gtc/matrix_transform.hpp"
#include "ShapeManager.h"

extern Engine* engine;

void AnimationScene::Init()
{
	demoModel.LoadModel("res/gh_sample_animation.fbx");

	light = new Light();
	light->position = glm::vec3(10.0, 100.0, 40.0);

	modelDraw = new Shader("src/animshaders/ModelDraw.vert", "src/animshaders/ModelDraw.frag");
}

void AnimationScene::Draw()
{
	modelDraw->Bind();
	modelDraw->SetUniform3f("lightPos", light->position.x, light->position.y, light->position.z);
	modelDraw->SetUniform3f("light", 3.2f, 3.2f, 3.2f);
	modelDraw->SetUniformMat4f("inverseview", glm::inverse(engine->pCamera->mView));
	modelDraw->SetUniformMat4f("projection", engine->pCamera->mProjection);
	modelDraw->SetUniformMat4f("view", engine->pCamera->mView);

	glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 7.5f, 0.0f));
	model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0, 0.0));
	modelDraw->SetUniformMat4f("model", model);
	modelDraw->SetUniformMat4f("normaltr", glm::inverse(model));
	modelDraw->SetUniform3f("diffuse", 1.0f, 0.0f, 0.0f);
	modelDraw->SetUniform3f("specular", 1.0f, 1.0f, 1.0f);
	modelDraw->SetUniform1f("shininess", 10.0f);

	demoModel.Draw(modelDraw);

	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
	model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0, 0.0, 0.0));
	model = glm::scale(model, glm::vec3(500.0f));
	modelDraw->SetUniformMat4f("model", model);
	modelDraw->SetUniformMat4f("normaltr", glm::inverse(model));
	modelDraw->SetUniform3f("diffuse", 0.0f, 1.0f, 0.0f);
	modelDraw->SetUniform3f("specular", 0.2f, 0.2f, 0.2f);
	modelDraw->SetUniform1f("shininess", 1.0f);
	std::pair<VertexArray*, ElementArrayBuffer*> shape = ShapeManager::Instance().mShapes[Shapes::QUAD];
	Renderer::Instance().Draw(*shape.first, *shape.second, *modelDraw);
}
