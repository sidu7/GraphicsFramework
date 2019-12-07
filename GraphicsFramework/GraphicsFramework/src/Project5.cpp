#include "Project5.h"
#include "../opengl/Shader.h"
#include "Light.h"
#include "ObjectManager.h"
#include <glm/gtc/matrix_transform.hpp>
#include "Engine.h"
#include "Camera.h"
#include "Object.h"
#include "Components.h"
#include "../opengl/FrameBuffer.h"
#include "imgui.h"

extern Engine* engine;

void Project5::Init()
{
	shader = new Shader("src/motionblur/GBuffer.vert", "src/motionblur/GBuffer.frag");

	lighting = new Shader("src/motionblur/Lighting.vert", "src/motionblur/Lighting.frag");

	ambient = new Shader("src/motionblur/Ambient.vert", "src/motionblur/Ambient.frag");

	light = new Light();
	light->position = glm::vec3(100.0f, 200.0f, 10.0f);
	light->distance = 200.0f;

	engine->pCamera->pitch = -15.0f;
	engine->pCamera->yaw = -279.3f;
	engine->pCamera->mCameraPos = glm::vec3(27.8f, 25.9f, -65.7f);
	engine->pCamera->mSpeed *= 3;
	engine->pCamera->CalculateFront();

	lighting->Bind();
	lighting->SetUniform3f("Light", 3.2f, 3.2f, 3.2f);

	ambient->Bind();
	ambient->SetUniform3f("Ambient", 0.1f, 0.1f, 0.1f);

	G_Buffer = new FrameBuffer(engine->scrWidth, engine->scrHeight, 5);
	Color = new FrameBuffer(engine->scrWidth, engine->scrHeight);

	gBuffershow = 0;

	//SkyDome shaders
	skyDomeShader = new Shader("src/shaders/SkyDome.vert", "src/shaders/SkyDome.frag");
	skyDomeTexture = new Texture("res/Textures/skyDome.hdr");
	exposure = 5.2f;
	contrast = 1.0f;

	// Load Objects in Scene
	ObjectManager::Instance().AddObject("res/JSON Data/Floor.json");
	//ObjectManager::Instance().AddObject("res/JSON Data/UpDown Teapot1.json");
	//ObjectManager::Instance().AddObject("res/JSON Data/UpDown Teapot2.json");
	ObjectManager::Instance().AddObject("res/JSON Data/LeftRight Teapot1.json");
	ObjectManager::Instance().AddObject("res/JSON Data/LeftRight Teapot2.json");
	ObjectManager::Instance().AddObject("res/JSON Data/Circle Teapot1.json");
	ObjectManager::Instance().AddObject("res/JSON Data/Circle Teapot2.json");
	//ObjectManager::Instance().AddObject("res/JSON Data/Teapot9.json");
	skyDome = ObjectManager::Instance().ReadObject("res/JSON Data/SkyDome.json");
	skyDome->pTransform->mModelTransformation = glm::scale(glm::mat4(1.0f), skyDome->pTransform->mScale);

	k = 20;
	S = 15;
	TileMax = new FrameBuffer(engine->scrWidth / k, engine->scrHeight / k);
	NeighbourMax = new FrameBuffer(engine->scrWidth / k, engine->scrHeight / k);

	TileMaxShader = new Shader("src/motionblur/TileMax.vert", "src/motionblur/TileMax.frag");
	NeighbourMaxShader = new Shader("src/motionblur/NeighbourMax.vert", "src/motionblur/NeighbourMax.frag");

	ReconstructionShader = new Shader("src/motionblur/Reconstruction.vert", "src/motionblur/Reconstruction.frag");
	MotionBlur = true;
	MotionBlurShader = new Shader("src/motionblur/MotionBlur.vert", "src/motionblur/MotionBlur.frag");
	ReconBlur = false;
	PerPixel = true;
}

void Project5::Draw()
{
	light->HandleInputs();

	ImGui::Begin("G-Buffer");	
	ImGui::InputFloat("Exposure", &exposure, 0.2);
	ImGui::InputFloat("Contrast", &contrast, 0.2);
	ImGui::Checkbox("Pause", &engine->stopMoving);
	ImGui::InputInt("Samples", &S);
	ImGui::Checkbox("MotionBlur", &MotionBlur);
	ImGui::End();

	//G-Buffer pass
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
	G_Buffer->Bind();
	shader->Bind();
	shader->SetUniformMat4f("view", engine->pCamera->mView);
	shader->SetUniformMat4f("prevview", engine->pCamera->mPrevView);
	shader->SetUniformMat4f("projection", engine->pCamera->mProjection);
	shader->SetUniform1f("deltaTime", Time::Instance().deltaTime);
	shader->SetUniform1i("k", k);

	ObjectManager::Instance().ObjectsDraw(shader);

	shader->Unbind();
	G_Buffer->Unbind();

	//Ambient light pass

	if (MotionBlur)
	{
		Color->Bind();
	}
	else
	{
		glViewport(0, 0, engine->scrWidth, engine->scrHeight);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);		
	}
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);

	ambient->Bind();
	G_Buffer->TexBind(2, 3);
	ambient->SetUniform1i("diffusetex", 3);
	Renderer::Instance().DrawQuad();
	ambient->Unbind();

	glEnable(GL_BLEND);
	// Global Lighting pass	
	lighting->Bind();
	G_Buffer->TexBind(0, 1);
	lighting->SetUniform1i("normaltex", 1);
	G_Buffer->TexBind(1, 2);
	lighting->SetUniform1i("worldpostex", 2);
	G_Buffer->TexBind(2, 3);
	lighting->SetUniform1i("diffusetex", 3);
	G_Buffer->TexBind(3, 4);
	lighting->SetUniform1i("specularalpha", 4);
	lighting->SetUniform1f("exposure", exposure);
	lighting->SetUniform1f("contrast", contrast);

	lighting->SetUniform3f("lightPos", light->position.x, light->position.y, light->position.z);
	lighting->SetUniformMat4f("inverseview", glm::inverse(engine->pCamera->mView));
	lighting->SetUniform1i("GBufferShow", gBuffershow);
	Renderer::Instance().DrawQuad();
	lighting->Unbind();

	glDisable(GL_CULL_FACE);
	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	
	// Copy depth contents from G-Buffer
	glBindFramebuffer(GL_READ_FRAMEBUFFER, G_Buffer->GetFrameBufferID());
	if(MotionBlur)
	{
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, Color->GetFrameBufferID());
	}
	else
	{
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);// write to default framebuffer		
	}
	glBlitFramebuffer(0, 0, G_Buffer->mWidth, G_Buffer->mHeight,
		0, 0, G_Buffer->mWidth, G_Buffer->mHeight, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
	if(MotionBlur)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, Color->GetFrameBufferID());
	}
	else
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);		
	}

	// Forward render Skydome
	skyDomeShader->Bind();
	skyDomeShader->SetUniformMat4f("view", engine->pCamera->mView);
	skyDomeShader->SetUniformMat4f("projection", engine->pCamera->mProjection);
	skyDomeShader->SetUniformMat4f("model", skyDome->pTransform->mModelTransformation);
	skyDomeTexture->Bind(1);
	skyDomeShader->SetUniform1i("skyDome", 1);
	skyDomeShader->SetUniform1f("exposure", exposure);
	skyDomeShader->SetUniform1f("contrast", contrast);
	Renderer::Instance().Draw(*skyDome->pShape->mShapeData.first, *skyDome->pShape->mShapeData.second, *skyDomeShader);
	skyDomeShader->Unbind();
	//engine->stopMoving = true;

	if (MotionBlur)
	{
		Color->Unbind();
		glDisable(GL_BLEND);
		glDisable(GL_DEPTH_TEST);

		
		MotionBlurShader->Bind();
		Color->TexBind(0, 1);
		MotionBlurShader->SetUniform1i("Color", 1);
		G_Buffer->TexBind(4, 2);
		MotionBlurShader->SetUniform1i("Velocity", 2);
		MotionBlurShader->SetUniform1i("S", S);
		Renderer::Instance().DrawQuad();

		MotionBlurShader->Unbind();
	}
}
