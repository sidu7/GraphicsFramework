#include "AllProjects.h"
#include <opengl/Shader.h>
#include <opengl/ComputeShader.h>
#include <opengl/Texture.h>
#include <opengl/FrameBuffer.h>
#include <opengl/UniformBuffer.h>

#include <core/Light.h>
#include <core/Engine.h>
#include <core/Camera.h>
#include <core/ObjectManager.h>
#include <core/Object.h>
#include <opengl/Renderer.h>
#include <core/Components/Transform.h>
#include <core/Components/Shape.h>

#include <iostream>
#include <ctime>
#include <Imgui/imgui.h>
#include <glm/gtc/matrix_transform.hpp>
#include <GLEW/include/GL/glew.h>

AllProjects::~AllProjects()
{
}

void AllProjects::Init()
{
	shader = new Shader("res/shaders/Drawing.vert", "res/shaders/Drawing.frag");

	lighting = new Shader("res/shaders/Lighting.vert", "res/shaders/Lighting.frag");

	ambient = new Shader("res/shaders/Ambient.vert", "res/shaders/Ambient.frag");

	shadow = new Shader("res/shaders/Shadow.vert", "res/shaders/Shadow.frag");

	locallight = new Shader("res/shaders/LocalLight.vert", "res/shaders/LocalLight.frag");

	blurHorizontal = new ComputeShader("res/shaders/BlurHorizontal.compute");
	blurVertical = new ComputeShader("res/shaders/BlurVertical.compute");

	light = new Light(glm::vec3(100.0f, 200.0f, 10.0f));

	Engine::Instance().pCamera->pitch = -15.0f;
	Engine::Instance().pCamera->yaw = -279.3f;
	Engine::Instance().pCamera->mCameraPos = glm::vec3(27.8f, 25.9f, -65.7f);
	Engine::Instance().pCamera->CalculateFront();

	lighting->Bind();
	lighting->SetUniform3f("Light", 1.0f, 1.0f, 1.0f);

	ambient->Bind();
	ambient->SetUniform3f("Ambient", 0.1f, 0.1f, 0.1f);

	G_Buffer = new FrameBuffer(Engine::Instance().scrWidth, Engine::Instance().scrHeight, 4);
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

	blurSize = 5;
	int s = blurSize / 2;
	float normB = 1.0f / (s * sqrt(3.141592f * 2));
	for (int i = -blurSize; i <= blurSize; ++i)
		//for (int i = 0; i <= blurSize; ++i)
	{
		float w = normB * exp(-(i * i) / (2.0f * s * s));
		blurWeights.emplace_back(w);
	}

	float sum = 0.0f;
	for (unsigned int i = 0; i < blurWeights.size(); ++i)
	{
		sum += blurWeights[i];
	}
	std::cout << "Weights total:" << sum << std::endl;

	block = new UniformBuffer(101 * sizeof(float));

	biasAlpha = 0.057f;

	//Fragment Shader blur
	blurShader = new Shader("res/shaders/Blur.vert", "res/shaders/Blur.frag");
	BlurFBO[0] = new FrameBuffer(ShadowMap->mWidth, ShadowMap->mHeight);
	BlurFBO[1] = new FrameBuffer(ShadowMap->mWidth, ShadowMap->mHeight);
	horizontalBlurred = new Texture(4, ShadowMap->mWidth, ShadowMap->mHeight);
	blurredShadowMap = new Texture(4, ShadowMap->mWidth, ShadowMap->mHeight);

	//SkyDome shaders
	skyDomeShader = new Shader("res/shaders/SkyDome.vert", "res/shaders/SkyDome.frag");
	skyDomeTexture = new Texture("res/Textures/skyDome.hdr");
	skyDomeIrradiance = new Texture("res/Textures/skyDomeIrr.hdr");
	exposure = 5.2f;
	contrast = 1.0f;

	// IBL
	Hblock.N = HBlockSize;
	HammersleyRandomPoints();
	HUniBlock = new UniformBuffer(sizeof(Hblock));
	HUniBlock->Bind(3);
	HUniBlock->SubData(sizeof(Hblock), &Hblock);
	IBLDiffuse = true;
	IBLSpecular = true;

	//SSAO
	AOShader = new Shader("res/shaders/AO.vert", "res/shaders/AO.frag");
	BlurredAO = new FrameBuffer(Engine::Instance().scrWidth, Engine::Instance().scrHeight);
	BilateralHorizontal = new ComputeShader("res/shaders/BilateralHorizontal.compute");
	BilateralVertical = new ComputeShader("res/shaders/BilateralVertical.compute");
	HorizontalBlurredAO = new Texture(4, Engine::Instance().scrWidth, Engine::Instance().scrHeight);
	ResultBlurredAO = new Texture(4, Engine::Instance().scrWidth, Engine::Instance().scrHeight);
	AONum = 15;
	AORadius = 10.0f;
	AOScale = 3.0f;
	AOContrast = 1.0f;

	// Load Objects in Scene
	ObjectManager::Instance().AddObject("res/JSON Data/Floor.json");
	ObjectManager::Instance().AddObject("res/JSON Data/Teapot1.json");
	ObjectManager::Instance().AddObject("res/JSON Data/Teapot2.json");
	ObjectManager::Instance().AddObject("res/JSON Data/Teapot3.json");
	ObjectManager::Instance().AddObject("res/JSON Data/Teapot4.json");
	ObjectManager::Instance().AddObject("res/JSON Data/Teapot5.json");
	ObjectManager::Instance().AddObject("res/JSON Data/Teapot6.json");
	ObjectManager::Instance().AddObject("res/JSON Data/Teapot7.json");
	ObjectManager::Instance().AddObject("res/JSON Data/Teapot8.json");
	//ObjectManager::Instance().AddObject("res/JSON Data/Teapot9.json");
	skyDome = ObjectManager::Instance().ReadObject("res/JSON Data/SkyDome.json");
	skyDome->GetComponent<Transform>()->mModelTransformation = glm::scale(glm::mat4(1.0f), skyDome->GetComponent<Transform>()->mScale);
	showLocalLights = true;
}

void AllProjects::Update()
{
	light->HandleInputs();

	

	//G-Buffer pass
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
	G_Buffer->Bind();
	shader->Bind();
	shader->SetUniformMat4f("view", Engine::Instance().pCamera->mView);
	shader->SetUniformMat4f("projection", Engine::Instance().pCamera->mProjection);

	ObjectManager::Instance().UpdateObjects(shader);

	shader->Unbind();
	G_Buffer->Unbind();

	//ShadowMap pass
	/*ShadowMap->Bind();
	shadow->Bind();

	glm::mat4 LightLookAt, shadowMatrix, LightProj;
	LightLookAt = glm::lookAt(light->position, glm::vec3(0.0f,0.0f,0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	LightProj = glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 2000.0f);

	shadow->SetUniformMat4f("projection", LightProj);
	shadow->SetUniformMat4f("view", LightLookAt);

	shadowMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.5f)) * glm::scale(glm::mat4(1.0f), glm::vec3(0.5f)) * LightProj * LightLookAt;

	ObjectManager::Instance().UpdateObjects(shadow);

	shadow->Unbind();
	ShadowMap->Unbind();

	//Blur Compute Shader
	block->Bind(2);
	block->SubData(sizeof(float) * blurWeights.size(), &blurWeights[0]);
	blurHorizontal->Bind();

	blurHorizontal->SetInputUniformImage("res", ShadowMap->m_TextureID[0], 0, 4);
	blurHorizontal->SetOutputUniformImage("dst", horizontalBlurred->GetTextureID(), 1, 4);
	blurHorizontal->SetUniform1i("width", blurSize);
	blurHorizontal->SetUniformBlock("blurKernel", 2);

	blurHorizontal->Run(ShadowMap->mWidth / 128, ShadowMap->mHeight, 1);
	blurHorizontal->Unbind();

	blurVertical->Bind();

	blurVertical->SetInputUniformImage("res", horizontalBlurred->GetTextureID(), 0, 4);
	blurVertical->SetOutputUniformImage("dst", blurredShadowMap->GetTextureID(), 1, 4);
	blurVertical->SetUniform1i("width", blurSize);
	blurVertical->SetUniformBlock("blurKernel", 2);

	blurVertical->Run(ShadowMap->mWidth, ShadowMap->mHeight/128, 1);
	blurVertical->Unbind();
	block->Unbind();

	//Blur Fragment Shader
	/*bool horizontal = true, first_iteration = true;
	unsigned int amount = blurSize + 1;
	block->Bind(2);
	block->SubData(sizeof(float) * blurWeights.size(), &blurWeights[0]);
	blurShader->Bind();
	blurShader->SetUniform1i("scene", 0);
	blurShader->SetUniform1i("blurSize", amount);
	blurShader->SetUniformBlock("blurKernel", 2);
	BlurFBO[1]->TexBind();
	BlurFBO[1]->Clear();
	for (unsigned int i = 0; i < amount; i++)
	{
		BlurFBO[horizontal]->Bind();
		blurShader->SetUniform1i("horizontal", horizontal);

		if (first_iteration)
			ShadowMap->TexBind(0, 0);
		else
			BlurFBO[!horizontal]->TexBind();

		Renderer::Instance().DrawQuad();
		horizontal = !horizontal;
		if (first_iteration)
			first_iteration = false;
	}
	BlurFBO[1]->Unbind();
	blurShader->Unbind();
	block->Unbind();*/
	//Blur Fragment Shader

	//IrradianceETerms->Bind(3);
	//irradianceCompute->Bind();
	////irradianceCompute->SetInputUniformImage("skydome", skyDomeTexture->GetTextureID(), 0, skyDomeTexture->mBPP);
	//skyDomeTexture->Bind();
	//irradianceCompute->SetUniform1i("skydome", 0);
	//irradianceCompute->SetUniform1i("width", skyDomeTexture->mWidth);
	//irradianceCompute->SetUniform1i("height", skyDomeTexture->mHeight);
	//irradianceCompute->Run(9, 1, 1);
	//ShaderStorageBuffer::PutMemoryBarrier();
	//IrradianceETerms->Bind(3);

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);

	//AO Pass
	BlurredAO->Bind();

	AOShader->Bind();
	G_Buffer->TexBind(0, 2);
	AOShader->SetUniform1i("normaltex", 2);
	G_Buffer->TexBind(1, 3);
	AOShader->SetUniform1i("worldpostex", 3);
	AOShader->SetUniform1f("contrast", AOContrast);
	AOShader->SetUniform1i("AOn", AONum);
	AOShader->SetUniform1f("AOR", AORadius);
	AOShader->SetUniform1f("AOscale", AOScale);
	Renderer::Instance().DrawQuad();
	AOShader->Unbind();

	BlurredAO->Unbind();

	//Bilateral Filter
	block->Bind(2);
	block->SubData(sizeof(float) * blurWeights.size(), &blurWeights[0]);
	BilateralHorizontal->Bind();

	BilateralHorizontal->SetInputUniformImage("res", BlurredAO->m_TextureID[0], 0, 4);
	BilateralHorizontal->SetInputUniformImage("normals", G_Buffer->m_TextureID[0], 1, 4);
	BilateralHorizontal->SetInputUniformImage("worldpos", G_Buffer->m_TextureID[1], 2, 4);
	BilateralHorizontal->SetOutputUniformImage("dst", HorizontalBlurredAO->GetTextureID(), 3, 4);
	BilateralHorizontal->SetUniform1i("width", blurSize);
	BilateralHorizontal->SetUniformBlock("blurKernel", 2);

	BilateralHorizontal->Run(BlurredAO->mWidth / 128, BlurredAO->mHeight, 1);
	BilateralHorizontal->Unbind();

	BilateralVertical->Bind();

	BilateralVertical->SetInputUniformImage("res", HorizontalBlurredAO->GetTextureID(), 0, 4);
	BilateralHorizontal->SetInputUniformImage("normals", G_Buffer->m_TextureID[0], 1, 4);
	BilateralHorizontal->SetInputUniformImage("worldpos", G_Buffer->m_TextureID[1], 2, 4);
	BilateralVertical->SetOutputUniformImage("dst", ResultBlurredAO->GetTextureID(), 3, 4);
	BilateralVertical->SetUniform1i("width", blurSize);
	BilateralVertical->SetUniformBlock("blurKernel", 2);

	BilateralVertical->Run(BlurredAO->mWidth, BlurredAO->mHeight / 128 + 1, 1);
	BilateralVertical->Unbind();
	block->Unbind();

	//Ambient light pass
	glViewport(0, 0, Engine::Instance().scrWidth, Engine::Instance().scrHeight);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);

	ambient->Bind();
	G_Buffer->TexBind(0, 2);
	ambient->SetUniform1i("normaltex", 2);
	G_Buffer->TexBind(1, 3);
	ambient->SetUniform1i("worldpostex", 3);
	G_Buffer->TexBind(2, 4);
	ambient->SetUniform1i("diffusetex", 4);
	G_Buffer->TexBind(3, 5);
	ambient->SetUniform1i("speculartex", 5);
	skyDomeIrradiance->Bind(6);
	ambient->SetUniform1i("irradiance", 6);
	skyDomeTexture->Bind(7);
	ambient->SetUniform1i("skydome", 7);
	ResultBlurredAO->Bind(8);
	//BlurredAO->TexBind(0, 8);
	ambient->SetUniform1i("AOtex", 8);
	ambient->SetUniform1f("exposure", exposure);
	ambient->SetUniform1f("contrast", contrast);
	ambient->SetUniform1i("showDiffuse", IBLDiffuse);
	ambient->SetUniform1i("showSpecular", IBLSpecular);
	ambient->SetUniformMat4f("inverseview", glm::inverse(Engine::Instance().pCamera->mView));
	ambient->SetUniformBlock("HBlock", 3);
	Renderer::Instance().DrawQuad();
	ambient->Unbind();

	G_Buffer->TexUnbind(0, 2);
	G_Buffer->TexUnbind(1, 3);
	G_Buffer->TexUnbind(2, 4);
	G_Buffer->TexUnbind(3, 5);
	skyDomeIrradiance->Unbind(6);
	skyDomeTexture->Unbind(7);
	//BlurredAO->TexUnbind(0, 8);
	ResultBlurredAO->Unbind(8);

	// Global Lighting pass
	/*if (gBuffershow == 0)
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
		blurredShadowMap->Bind(5);
		//ShadowMap->TexBind(0, 5);
		//BlurFBO[!horizontal]->TexBind(0, 5);
		lighting->SetUniform1i("shadowmap", 5);
		lighting->SetUniformMat4f("shadowmat", shadowMatrix);
		lighting->SetUniform1f("biasAlpha", biasAlpha);
		lighting->SetUniform1f("exposure", exposure);
		lighting->SetUniform1f("contrast", contrast);

		lighting->SetUniform3f("lightPos", light->position.x, light->position.y, light->position.z);
		lighting->SetUniformMat4f("inverseview", glm::inverse(Engine::Instance().pCamera->mView));
		lighting->SetUniform1i("GBufferShow", gBuffershow);
		Renderer::Instance().DrawQuad();
		lighting->Unbind();
	}

	// Local Lighting pass
	if (gBuffershow == 0 && showLocalLights)
	{
		glEnable(GL_CULL_FACE);
		glCullFace(GL_FRONT);

		locallight->Bind();
		locallight->SetUniformMat4f("view", Engine::Instance().pCamera->mView);
		locallight->SetUniformMat4f("projection", Engine::Instance().pCamera->mProjection);
		G_Buffer->TexBind(0, 1);
		locallight->SetUniform1i("normaltex", 1);
		G_Buffer->TexBind(1, 2);
		locallight->SetUniform1i("worldpostex", 2);
		G_Buffer->TexBind(2, 3);
		locallight->SetUniform1i("diffusetex", 3);
		G_Buffer->TexBind(3, 4);
		locallight->SetUniform1i("specularalpha", 4);
		locallight->SetUniformMat4f("inverseview", glm::inverse(Engine::Instance().pCamera->mView));
		float lightRadius = 4.0f;
		locallight->SetUniform1f("lightRadius", lightRadius);
		for (unsigned int i = 0; i < 40; ++i)
		{
			for (unsigned int j = 0; j < 40; ++j)
			{
				glm::mat4 model = glm::mat4(1.0f);
				model = glm::translate(model, glm::vec3(i * lightRadius * 2 - 40.0f * lightRadius, lightRadius / 2, j * lightRadius * 2 - 40.0f * lightRadius));
				model = glm::scale(model, glm::vec3(lightRadius));
				locallight->SetUniform3f("lightPos", i * lightRadius * 2 - 40.0f * lightRadius, lightRadius / 2, j * lightRadius * 2 - 40.0f * lightRadius);
				locallight->SetUniformMat4f("model", model);
				locallight->SetUniform3f("Light", lightColors[i][j].x, lightColors[i][j].y, lightColors[i][j].z);
				std::pair<VertexArray*, ElementArrayBuffer*> shape = ShapeManager::Instance().mShapes[Shapes::SPHERE];
				Renderer::Instance().Draw(*shape.first, *shape.second, *locallight);
			}
		}
	}*/
	glDisable(GL_CULL_FACE);
	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);

	// Copy depth contents from G-Buffer
	glBindFramebuffer(GL_READ_FRAMEBUFFER, G_Buffer->GetFrameBufferID());
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);// write to default framebuffer
	glBlitFramebuffer(0, 0, G_Buffer->mWidth, G_Buffer->mHeight,
		0, 0, G_Buffer->mWidth, G_Buffer->mHeight, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

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
	Engine::Instance().stopMoving = true;
}

void AllProjects::DebugDisplay()
{
	ImGui::Begin("G-Buffer");
	ImGui::Checkbox("Lighting", &lighton);
	ImGui::Checkbox("Local Lights", &showLocalLights);
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
	ImGui::DragFloat3("Light Position", &light->position[0], 2.0f, -300.0f, 300.0f);
	ImGui::InputFloat("biasAlpha", &biasAlpha, 0.0005);
	if (ImGui::InputInt("BlurSize", &blurSize))
	{
		int s = blurSize / 2;
		float normB = 1.0f / (s * sqrt(3.141592f * 2));
		blurWeights.clear();
		for (int i = -blurSize; i <= blurSize; ++i)
		{
			float w = normB * exp(-(i * i) / (2.0f * s * s));
			blurWeights.emplace_back(w);
		}
	}
	ImGui::InputFloat("Exposure", &exposure, 0.2);
	ImGui::InputFloat("Contrast", &contrast, 0.2);
	ImGui::Checkbox("Pause Moving", &Engine::Instance().stopMoving);
	ImGui::Checkbox("Show IBL Diffuse", &IBLDiffuse);
	ImGui::Checkbox("Show IBL Specular", &IBLSpecular);
	ImGui::InputInt("AONum", &AONum);
	ImGui::InputFloat("AORadius", &AORadius, 1.0f);
	ImGui::InputFloat("AOScale", &AOScale, 0.1f);
	ImGui::InputFloat("AOContrast", &AOContrast, 0.1f);
	ImGui::End();
}

void AllProjects::HammersleyRandomPoints()
{
	int kk;
	int pos = 0;
	float p, u;

	for (int k = 0; k < Hblock.N; ++k)
	{
		for (p = 0.5f, kk = k, u = 0.0f; kk; p *= 0.5f, kk >>= 1)
		{
			if (kk & 1)
			{
				u += p;
			}
		}
		float v = (k + 0.5) / Hblock.N;
		Hblock.hammersley[pos++] = u;
		Hblock.hammersley[pos++] = v;
	}
}
