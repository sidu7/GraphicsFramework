#include "MotionBlur.h"

#include "core/Engine.h"
#include "core/Camera.h"
#include "core/Light.h"
#include "core/ObjectManager.h"
#include "core/Time.h"
#include "core/Object.h"
#include "core/Components/Transform.h"
#include "core/Components/Shape.h"
#include "opengl/Shader.h"
#include "opengl/FrameBuffer.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <Imgui/imgui.h>

MotionBlur::~MotionBlur()
{
}

void MotionBlur::Init()
{
	shader = new Shader("res/motionblur/GBuffer.vert", "res/motionblur/GBuffer.frag");

	lighting = new Shader("res/motionblur/Lighting.vert", "res/motionblur/Lighting.frag");

	ambient = new Shader("res/motionblur/Ambient.vert", "res/motionblur/Ambient.frag");

	light = new Light(glm::vec3(100.0f, 200.0f, 10.0f));

	Engine::Instance().pCamera->pitch = -15.0f;
	Engine::Instance().pCamera->yaw = -279.3f;
	Engine::Instance().pCamera->mCameraPos = glm::vec3(27.8f, 25.9f, -65.7f);
	Engine::Instance().pCamera->mSpeed *= 3;
	Engine::Instance().pCamera->CalculateFront();

	lighting->Bind();
	lighting->SetUniform3f("Light", 3.2f, 3.2f, 3.2f);

	ambient->Bind();
	ambient->SetUniform3f("Ambient", 0.1f, 0.1f, 0.1f);

	G_Buffer = new FrameBuffer(Engine::Instance().scrWidth, Engine::Instance().scrHeight, 5);
	Color = new FrameBuffer(Engine::Instance().scrWidth, Engine::Instance().scrHeight);

	gBuffershow = 0;

	//SkyDome shaders
	skyDomeShader = new Shader("res/shaders/SkyDome.vert", "res/shaders/SkyDome.frag");
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
	skyDome->GetComponent<Transform>()->mModelTransformation = glm::scale(glm::mat4(1.0f), skyDome->GetComponent<Transform>()->mScale);

	k = 20;
	S = 15;
	TileMax = new FrameBuffer(Engine::Instance().scrWidth / k, Engine::Instance().scrHeight / k);
	NeighbourMax = new FrameBuffer(Engine::Instance().scrWidth / k, Engine::Instance().scrHeight / k);

	TileMaxShader = new Shader("res/motionblur/TileMax.vert", "res/motionblur/TileMax.frag");
	NeighbourMaxShader = new Shader("res/motionblur/NeighbourMax.vert", "res/motionblur/NeighbourMax.frag");

	ReconstructionShader = new Shader("res/motionblur/Reconstruction.vert", "res/motionblur/Reconstruction.frag");
	MotionBlurOn = true;
	MotionBlurShader = new Shader("res/motionblur/MotionBlur.vert", "res/motionblur/MotionBlur.frag");
	ReconBlur = false;
	PerPixel = true;
}

void MotionBlur::Update()
{
	light->HandleInputs();

	//G-Buffer pass
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
	G_Buffer->Bind();
	shader->Bind();
	shader->SetUniformMat4f("view", Engine::Instance().pCamera->mView);
	shader->SetUniformMat4f("prevview", Engine::Instance().pCamera->mPrevView);
	shader->SetUniformMat4f("projection", Engine::Instance().pCamera->mProjection);
	shader->SetUniform1f("deltaTime", Time::Instance().deltaTime);
	shader->SetUniform1i("k", k);

	ObjectManager::Instance().ObjectsDraw(shader);

	shader->Unbind();
	G_Buffer->Unbind();

	//Ambient light pass

	if (MotionBlurOn)
	{
		Color->Bind();
	}
	else
	{
		glViewport(0, 0, Engine::Instance().scrWidth, Engine::Instance().scrHeight);
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
	lighting->SetUniformMat4f("inverseview", glm::inverse(Engine::Instance().pCamera->mView));
	lighting->SetUniform1i("GBufferShow", gBuffershow);
	Renderer::Instance().DrawQuad();
	lighting->Unbind();

	glDisable(GL_CULL_FACE);
	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);

	// Copy depth contents from G-Buffer
	glBindFramebuffer(GL_READ_FRAMEBUFFER, G_Buffer->GetFrameBufferID());
	if (MotionBlurOn)
	{
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, Color->GetFrameBufferID());
	}
	else
	{
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);// write to default framebuffer		
	}
	glBlitFramebuffer(0, 0, G_Buffer->mWidth, G_Buffer->mHeight,
		0, 0, G_Buffer->mWidth, G_Buffer->mHeight, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
	if (MotionBlurOn)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, Color->GetFrameBufferID());
	}
	else
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	// Forward render Skydome
	skyDomeShader->Bind();
	skyDomeShader->SetUniformMat4f("view", Engine::Instance().pCamera->mView);
	skyDomeShader->SetUniformMat4f("projection", Engine::Instance().pCamera->mProjection);
	skyDomeShader->SetUniformMat4f("model", skyDome->GetComponent<Transform>()->mModelTransformation);
	skyDomeTexture->Bind(1);
	skyDomeShader->SetUniform1i("skyDome", 1);
	skyDomeShader->SetUniform1f("exposure", exposure);
	skyDomeShader->SetUniform1f("contrast", contrast);
	Renderer::Instance().Draw(*skyDome->GetComponent<Shape>()->mShapeData.first, *skyDome->GetComponent<Shape>()->mShapeData.second, *skyDomeShader);
	skyDomeShader->Unbind();
	//Engine::Instance().stopMoving = true;

	if (MotionBlurOn)
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

void MotionBlur::DebugDisplay()
{
	ImGui::Begin("G-Buffer");
	ImGui::InputFloat("Exposure", &exposure, 0.2);
	ImGui::InputFloat("Contrast", &contrast, 0.2);
	ImGui::Checkbox("Pause", &Engine::Instance().stopMoving);
	ImGui::InputInt("Samples", &S);
	ImGui::Checkbox("MotionBlur", &MotionBlurOn);
	ImGui::End();
}
