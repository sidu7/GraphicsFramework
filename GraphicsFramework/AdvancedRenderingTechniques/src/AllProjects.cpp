#include "AllProjects.h"
#include "Rendering/Shader.h"
#include "Rendering/ComputeShader.h"
#include "Rendering/Texture.h"
#include "Rendering/FrameBuffer.h"
#include "Rendering/UniformBuffer.h"
#include "Rendering/Renderer.h"
#include "Rendering/RenderingFactory.h"

#include "Core/Light.h"
#include "Core/Engine.h"
#include "Core/Window.h"
#include "Core/Camera.h"
#include "Core/ObjectManager.h"
#include "Core/ShapeManager.h"
#include "Core/Object.h"
#include "Core/Components/Transform.h"
#include "Core/Components/Shape.h"
#include "Core/Components/Material.h"

#include <iostream>
#include <Imgui/imgui.h>
#include <glm/gtc/matrix_transform.hpp>
#include <GLEW/include/GL/glew.h>

#define MAX_BLUR_WEIGHTS_NUM 101

struct Matrices
{
	glm::mat4 mView;
	glm::mat4 mProjection;
};

AllProjects::~AllProjects()
{
}

void AllProjects::Init()
{
	baseShader = RenderingFactory::Instance()->CreateShader();
	baseShader->Init("res/shaders/Drawing.vert", "res/shaders/Drawing.frag");

	lighting = RenderingFactory::Instance()->CreateShader(); 
	lighting->Init("res/shaders/Lighting.vert", "res/shaders/Lighting.frag");

	ambient = RenderingFactory::Instance()->CreateShader(); 
	ambient->Init("res/shaders/Ambient.vert", "res/shaders/Ambient.frag");

	ambientNoAO = RenderingFactory::Instance()->CreateShader();
	ambientNoAO->Init("res/shaders/Ambient.vert", "res/shaders/AmbientNoAO.frag");

	shadow = RenderingFactory::Instance()->CreateShader(); 
	shadow->Init("res/shaders/Shadow.vert", "res/shaders/Shadow.frag");

	locallight = RenderingFactory::Instance()->CreateShader(); 
	locallight->Init("res/shaders/LocalLight.vert", "res/shaders/LocalLight.frag");

	globalMatrices = RenderingFactory::Instance()->CreateUniformBuffer();
	globalMatrices->Init(sizeof(Matrices));

	blurHorizontal = RenderingFactory::Instance()->CreateComputeShader(); 
	blurHorizontal->SetShader("res/shaders/BlurHorizontal.compute");
	blurVertical = RenderingFactory::Instance()->CreateComputeShader(); 
	blurVertical->SetShader("res/shaders/BlurVertical.compute");

	light = new Light(glm::vec3(100.0f, 200.0f, 10.0f));

	Engine::Instance()->GetCamera()->pitch = -15.0f;
	Engine::Instance()->GetCamera()->yaw = -279.3f;
	Engine::Instance()->GetCamera()->mCameraPos = glm::vec3(27.8f, 25.9f, -65.7f);
	Engine::Instance()->GetCamera()->CalculateFront();

	Renderer::Instance()->BindShader(lighting);
	lighting->SetUniform3f("Light", 1.0f, 1.0f, 1.0f);

	Renderer::Instance()->BindShader(ambient);
	ambient->SetUniform3f("Ambient", 0.1f, 0.1f, 0.1f);

	Renderer::Instance()->BindShader(ambientNoAO);
	ambientNoAO->SetUniform3f("Ambient", 0.1f, 0.1f, 0.1f);

	showAO = true;

	G_Buffer = RenderingFactory::Instance()->CreateFrameBuffer(); 
	G_Buffer->Init(Window::Instance()->GetWidth(), Window::Instance()->GetHeight(), 4);
	ShadowMap = RenderingFactory::Instance()->CreateFrameBuffer(); 
	ShadowMap->Init(1024, 1024);

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

	block = RenderingFactory::Instance()->CreateUniformBuffer(); 
	block->Init(MAX_BLUR_WEIGHTS_NUM * sizeof(float));

	biasAlpha = 0.057f;

	// Shadow blur
	horizontalBlurred = RenderingFactory::Instance()->CreateTexture();
	horizontalBlurred->Init(4, ShadowMap->GetWidth(), ShadowMap->GetHeight());
	blurredShadowMap = RenderingFactory::Instance()->CreateTexture(); 
	blurredShadowMap->Init(4, ShadowMap->GetWidth(), ShadowMap->GetHeight());

	//SkyDome shaders
	skyDomeShader = RenderingFactory::Instance()->CreateShader(); 
	skyDomeShader->Init("res/shaders/SkyDome.vert", "res/shaders/SkyDome.frag");
	skyDomeTexture = RenderingFactory::Instance()->CreateTexture(); 
	skyDomeTexture->Init("res/Textures/popDome.hdr");
	skyDomeIrradiance = RenderingFactory::Instance()->CreateTexture(); 
	skyDomeIrradiance->Init("res/Textures/popDomeIrr.hdr");
	exposure = 5.2f;
	contrast = 1.0f;

	// IBL
	Hblock.N = HBlockSize;
	HammersleyRandomPoints();
	HUniBlock = RenderingFactory::Instance()->CreateUniformBuffer(); 
	HUniBlock->Init(sizeof(Hblock));
	HUniBlock->AddData(sizeof(Hblock), &Hblock);
	IBLDiffuse = true;
	IBLSpecular = true;

	//SSAO
	AOShader = RenderingFactory::Instance()->CreateShader(); 
	AOShader->Init("res/shaders/AO.vert", "res/shaders/AO.frag");
	BlurredAO = RenderingFactory::Instance()->CreateFrameBuffer(); 
	BlurredAO->Init(Window::Instance()->GetWidth(), Window::Instance()->GetHeight());
	BilateralHorizontal = RenderingFactory::Instance()->CreateComputeShader(); 
	BilateralHorizontal->SetShader("res/shaders/BilateralHorizontal.compute");
	BilateralVertical = RenderingFactory::Instance()->CreateComputeShader(); 
	BilateralVertical->SetShader("res/shaders/BilateralVertical.compute");
	HorizontalBlurredAO = RenderingFactory::Instance()->CreateTexture(); 
	HorizontalBlurredAO->Init(4, Window::Instance()->GetWidth(), Window::Instance()->GetHeight());
	ResultBlurredAO = RenderingFactory::Instance()->CreateTexture(); 
	ResultBlurredAO->Init(4, Window::Instance()->GetWidth(), Window::Instance()->GetHeight());
	AONum = 115;
	AORadius = 10.0f;
	AOScale = 3.0f;
	AOContrast = 1.0f;

	// Load Objects in Scene
	ObjectManager::Instance()->AddObject("res/JSON Data/Floor.json");
	ObjectManager::Instance()->AddObject("res/JSON Data/Teapot1.json");
	ObjectManager::Instance()->AddObject("res/JSON Data/Teapot2.json");
	ObjectManager::Instance()->AddObject("res/JSON Data/Teapot3.json");
	ObjectManager::Instance()->AddObject("res/JSON Data/Teapot4.json");
	ObjectManager::Instance()->AddObject("res/JSON Data/Teapot5.json");
	ObjectManager::Instance()->AddObject("res/JSON Data/Teapot6.json");
	ObjectManager::Instance()->AddObject("res/JSON Data/Teapot7.json");
	ObjectManager::Instance()->AddObject("res/JSON Data/Teapot8.json");
	//ObjectManager::Instance().AddObject("res/JSON Data/Teapot9.json");
	skyDome = ObjectManager::Instance()->ReadObject("res/JSON Data/SkyDome.json");
	skyDome->GetComponent<Transform>()->mModelTransformation = glm::scale(glm::mat4(1.0f), skyDome->GetComponent<Transform>()->mScale);
	showLocalLights = true;
}

void AllProjects::Close()
{
	delete baseShader;
	delete lighting;
	delete ambient;
	delete ambientNoAO;
	delete shadow;
	delete locallight;
	delete globalMatrices;
	delete blurHorizontal;
	delete blurVertical;
	delete light;

	delete G_Buffer;
	delete ShadowMap;

	memset(lightColors, 0, sizeof(lightColors));	
	blurWeights.clear();

	delete block;

	// Shadow blur
	delete horizontalBlurred;
	delete blurredShadowMap;

	//SkyDome shaders
	delete skyDomeShader;
	delete skyDomeTexture;
	delete skyDomeIrradiance;

	// IBL
	memset(&Hblock, 0, sizeof(Hblock));
	HammersleyRandomPoints();
	delete HUniBlock;

	//SSAO
	delete AOShader;
	delete BlurredAO;
	delete BilateralHorizontal;
	delete BilateralVertical;
	delete HorizontalBlurredAO;
	delete ResultBlurredAO;

	// Clear Objects in Scene
	ObjectManager::Instance()->ClearObjects();
}

void AllProjects::Update()
{
	light->HandleInputs();

	ObjectManager::Instance()->UpdateObjects();
	
	Renderer* pRenderer = Renderer::Instance();

	//G-Buffer pass
	pRenderer->SetDepthTest(true);
	pRenderer->SetBlending(false);

	pRenderer->BindFrameBuffer(G_Buffer);
	G_Buffer->Clear();
	pRenderer->BindShader(baseShader);

	Matrices MatData{ Engine::Instance()->GetCamera()->mView , Engine::Instance()->GetCamera()->mProjection };
	globalMatrices->AddData(sizeof(MatData), &MatData);
	pRenderer->BindUniformBuffer(globalMatrices, 0);

	ObjectManager::Instance()->RenderObjects(baseShader);

	pRenderer->UnbindShader(baseShader);
	pRenderer->UnbindFrameBuffer(G_Buffer);
	pRenderer->UnbindUniformBuffer(globalMatrices);

	//ShadowMap pass
	pRenderer->BindFrameBuffer(ShadowMap);
	ShadowMap->Clear();
	pRenderer->BindShader(shadow);

	glm::mat4 LightLookAt, shadowMatrix, LightProj;
	LightLookAt = glm::lookAt(light->position, glm::vec3(0.0f,0.0f,0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	LightProj = glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 2000.0f);

	Matrices ShadowMatricesData{ LightLookAt , LightProj };
	globalMatrices->AddData(sizeof(ShadowMatricesData), &ShadowMatricesData);
	pRenderer->BindUniformBuffer(globalMatrices, 0);

	shadowMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.5f)) * glm::scale(glm::mat4(1.0f), glm::vec3(0.5f)) * LightProj * LightLookAt;

	ObjectManager::Instance()->RenderObjects(shadow);

	pRenderer->UnbindShader(shadow);
	pRenderer->UnbindFrameBuffer(ShadowMap);

	//Blur Compute Shader
	if (softShadows)
	{
		pRenderer->BindUniformBuffer(block, 2);
		block->AddData(sizeof(float) * blurWeights.size(), &blurWeights[0]);
		pRenderer->BindComputeShader(blurHorizontal);

		blurHorizontal->SetInputUniformImage("res", ShadowMap->GetTexture(0), 0, 4);
		blurHorizontal->SetOutputUniformImage("dst", horizontalBlurred, 1, 4);
		blurHorizontal->SetUniform1i("width", blurSize);
		blurHorizontal->SetUniformBlock("blurKernel", 2);

		blurHorizontal->Run(ShadowMap->GetWidth() / 128, ShadowMap->GetHeight(), 1);
		pRenderer->UnbindComputeShader(blurHorizontal);

		pRenderer->BindComputeShader(blurVertical);

		blurVertical->SetInputUniformImage("res", horizontalBlurred, 0, 4);
		blurVertical->SetOutputUniformImage("dst", blurredShadowMap, 1, 4);
		blurVertical->SetUniform1i("width", blurSize);
		blurVertical->SetUniformBlock("blurKernel", 2);

		blurVertical->Run(ShadowMap->GetWidth(), ShadowMap->GetHeight() / 128, 1);
		pRenderer->UnbindComputeShader(blurVertical);
		pRenderer->UnbindUniformBuffer(block);
	}

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

	pRenderer->SetDepthTest(false);
	pRenderer->SetBlending(false);

	//AO Pass
	pRenderer->BindFrameBuffer(BlurredAO);
	BlurredAO->Clear();

	pRenderer->BindShader(AOShader);
	G_Buffer->TexBind(0, 2);
	AOShader->SetUniform1i("normaltex", 2);
	G_Buffer->TexBind(1, 3);
	AOShader->SetUniform1i("worldpostex", 3);
	AOShader->SetUniform1f("contrast", AOContrast);
	AOShader->SetUniform1i("AOn", AONum);
	AOShader->SetUniform1f("AOR", AORadius);
	AOShader->SetUniform1f("AOscale", AOScale);
	Renderer::Instance()->DrawQuad(AOShader);
	pRenderer->UnbindShader(AOShader);

	pRenderer->UnbindFrameBuffer(BlurredAO);

	//Bilateral Filter
	pRenderer->BindUniformBuffer(block, 2);
	block->AddData(sizeof(float) * blurWeights.size(), &blurWeights[0]);
	pRenderer->BindComputeShader(BilateralHorizontal);

	BilateralHorizontal->SetInputUniformImage("res", BlurredAO->GetTexture(0), 0, 4);
	BilateralHorizontal->SetInputUniformImage("normals", G_Buffer->GetTexture(0), 1, 4);
	BilateralHorizontal->SetInputUniformImage("worldpos", G_Buffer->GetTexture(1), 2, 4);
	BilateralHorizontal->SetOutputUniformImage("dst", HorizontalBlurredAO, 3, 4);
	BilateralHorizontal->SetUniform1i("width", blurSize);
	BilateralHorizontal->SetUniformBlock("blurKernel", 2);

	BilateralHorizontal->Run(BlurredAO->GetWidth() / 128, BlurredAO->GetHeight(), 1);
	pRenderer->UnbindComputeShader(BilateralHorizontal);

	pRenderer->BindComputeShader(BilateralVertical);

	BilateralVertical->SetInputUniformImage("res", HorizontalBlurredAO, 0, 4);
	BilateralHorizontal->SetInputUniformImage("normals", G_Buffer->GetTexture(0), 1, 4);
	BilateralHorizontal->SetInputUniformImage("worldpos", G_Buffer->GetTexture(1), 2, 4);
	BilateralVertical->SetOutputUniformImage("dst", ResultBlurredAO, 3, 4);
	BilateralVertical->SetUniform1i("width", blurSize);
	BilateralVertical->SetUniformBlock("blurKernel", 2);

	BilateralVertical->Run(BlurredAO->GetWidth(), BlurredAO->GetHeight() / 128 + 1, 1);
	pRenderer->UnbindComputeShader(BilateralVertical);
	pRenderer->UnbindUniformBuffer(block);

	//Ambient light pass
	pRenderer->SetViewportSize(glm::vec2(0, 0), glm::vec2(Window::Instance()->GetWidth(), Window::Instance()->GetHeight()));
	pRenderer->Clear();

	pRenderer->SetDepthTest(false);
	pRenderer->SetBlending(false);

	Shader* ambientShader = showAO ? ambient : ambientNoAO;
	pRenderer->BindShader(ambientShader);
	G_Buffer->TexBind(0, 2);
	ambientShader->SetUniform1i("normaltex", 2);
	G_Buffer->TexBind(1, 3);
	ambientShader->SetUniform1i("worldpostex", 3);
	G_Buffer->TexBind(2, 4);
	ambientShader->SetUniform1i("diffusetex", 4);
	G_Buffer->TexBind(3, 5);
	ambientShader->SetUniform1i("speculartex", 5);
	pRenderer->BindTexture(skyDomeIrradiance, 6);
	ambientShader->SetUniform1i("irradiance", 6);
	pRenderer->BindTexture(skyDomeTexture, 7);
	ambientShader->SetUniform1i("skydome", 7);

	if (showAO)
	{
		pRenderer->BindTexture(ResultBlurredAO, 8);
		//BlurredAO->TexBind(0, 8);
		ambientShader->SetUniform1i("AOtex", 8);
	}
	ambientShader->SetUniform1f("exposure", exposure);
	ambientShader->SetUniform1f("contrast", contrast);
	ambientShader->SetUniform1i("showDiffuse", IBLDiffuse);
	ambientShader->SetUniform1i("showSpecular", IBLSpecular);
	ambientShader->SetUniformMat4f("inverseview", glm::inverse(Engine::Instance()->GetCamera()->mView));
	Renderer::Instance()->BindUniformBuffer(HUniBlock, 5);
	Renderer::Instance()->DrawQuad(ambientShader);
	Renderer::Instance()->UnbindUniformBuffer(HUniBlock);
	pRenderer->UnbindShader(ambientShader);

	G_Buffer->TexUnbind(0, 2);
	G_Buffer->TexUnbind(1, 3);
	G_Buffer->TexUnbind(2, 4);
	G_Buffer->TexUnbind(3, 5);
	pRenderer->UnbindTexture(skyDomeIrradiance, 6);
	pRenderer->UnbindTexture(skyDomeTexture, 7);
	//BlurredAO->TexUnbind(0, 8);
	pRenderer->UnbindTexture(ResultBlurredAO, 8);

	// Global Lighting pass
	if (gBuffershow == 0)
	{
		pRenderer->SetBlending(true);
	}

	if (lighton)
	{
		pRenderer->BindShader(lighting);
		G_Buffer->TexBind(0, 1);
		lighting->SetUniform1i("normaltex", 1);
		G_Buffer->TexBind(1, 2);
		lighting->SetUniform1i("worldpostex", 2);
		G_Buffer->TexBind(2, 3);
		lighting->SetUniform1i("diffusetex", 3);
		G_Buffer->TexBind(3, 4);
		lighting->SetUniform1i("specularalpha", 4);
		if (softShadows)
		{
			pRenderer->BindTexture(blurredShadowMap, 5);
		}
		else
		{
			pRenderer->BindTexture(ShadowMap->GetTexture(), 5);
		}
		lighting->SetUniform1i("shadowmap", 5);
		lighting->SetUniformMat4f("shadowmat", shadowMatrix);
		lighting->SetUniform1f("biasAlpha", biasAlpha);
		lighting->SetUniform1f("exposure", exposure);
		lighting->SetUniform1f("contrast", contrast);

		lighting->SetUniform3f("lightPos", light->position.x, light->position.y, light->position.z);
		lighting->SetUniformMat4f("inverseview", glm::inverse(Engine::Instance()->GetCamera()->mView));
		lighting->SetUniform1i("GBufferShow", gBuffershow);
		Renderer::Instance()->DrawQuad(lighting);
		pRenderer->UnbindShader(lighting);
	}

	// Local Lighting pass
	if (gBuffershow == 0 && showLocalLights)
	{
		pRenderer->SetCullingFace(CullFace::Front);

		pRenderer->BindShader(locallight);
		locallight->SetUniformMat4f("view", Engine::Instance()->GetCamera()->mView);
		locallight->SetUniformMat4f("projection", Engine::Instance()->GetCamera()->mProjection);
		G_Buffer->TexBind(0, 1);
		locallight->SetUniform1i("normaltex", 1);
		G_Buffer->TexBind(1, 2);
		locallight->SetUniform1i("worldpostex", 2);
		G_Buffer->TexBind(2, 3);
		locallight->SetUniform1i("diffusetex", 3);
		G_Buffer->TexBind(3, 4);
		locallight->SetUniform1i("specularalpha", 4);
		locallight->SetUniformMat4f("inverseview", glm::inverse(Engine::Instance()->GetCamera()->mView));
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
				ShapeData& shape = ShapeManager::Instance()->mShapes[Shapes::SPHERE];
				Renderer::Instance()->Draw(shape.mVBO, shape.mIBO, locallight);
			}
		}
	}

	pRenderer->SetCullingFace(CullFace::None);
	pRenderer->SetBlending(false);
	pRenderer->SetDepthTest(true);

	// Copy depth contents from G-Buffer
	G_Buffer->CopyDepthTo(Renderer::Instance()->GetBackBuffer());

	// Forward render Skydome
	pRenderer->BindShader(skyDomeShader);
	skyDomeShader->SetUniformMat4f("view", Engine::Instance()->GetCamera()->mView);
	skyDomeShader->SetUniformMat4f("projection", Engine::Instance()->GetCamera()->mProjection);
	skyDomeShader->SetUniformMat4f("model", skyDome->GetComponent<Transform>()->mModelTransformation);
	pRenderer->BindTexture(skyDomeTexture, 1);
	skyDomeShader->SetUniform1i("skyDome", 1);
	skyDomeShader->SetUniform1f("exposure", exposure);
	skyDomeShader->SetUniform1f("contrast", contrast);
	Renderer::Instance()->Draw(skyDome->GetComponent<Shape>()->mShapeData->mVBO, skyDome->GetComponent<Shape>()->mShapeData->mIBO, skyDomeShader);
	pRenderer->UnbindShader(skyDomeShader);
	Engine::Instance()->stopMoving = true;
}

void AllProjects::DebugDisplay()
{
	ImGui::Begin("G-Buffer");
	ImGui::Checkbox("Lighting", &lighton);
	ImGui::Checkbox("Local Lights", &showLocalLights);
	ImGui::Checkbox("Show AO", &showAO);
	ImGui::Checkbox("Soft Shadows", &softShadows);

	static const char* bufferList[] = {
		"None",
		"Normals",
		"WorldPos",
		"Diffuse",
		"Specular"
	};

	bool selected = true;
	if (ImGui::BeginCombo("Select G-Buffer", bufferList[gBuffershow]))
	{
		lighton = true;
		for (int i = 0; i < ARRAY_SIZE(bufferList); ++i)
		{
			if (ImGui::Selectable(bufferList[i], selected))
				gBuffershow = i;
		}
		ImGui::EndCombo();
	}
	ImGui::DragFloat3("Light Position", &light->position[0], 2.0f, -300.0f, 300.0f);
	ImGui::InputFloat("biasAlpha", &biasAlpha, 0.0005);
	if (ImGui::SliderInt("BlurSize", &blurSize, 2, MAX_BLUR_WEIGHTS_NUM / 2))
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
	ImGui::Checkbox("Pause Moving", &Engine::Instance()->stopMoving);
	ImGui::Checkbox("Show IBL Diffuse", &IBLDiffuse);
	ImGui::Checkbox("Show IBL Specular", &IBLSpecular);
	ImGui::InputInt("AONum", &AONum);
	ImGui::InputFloat("AORadius", &AORadius, 1.0f);
	ImGui::InputFloat("AOScale", &AOScale, 0.1f);
	ImGui::InputFloat("AOContrast", &AOContrast, 0.1f);
	ImGui::End();
}

void AllProjects::RenderObject(Object* object, Shader* shader)
{
	Transform* transform = object->GetComponent<Transform>();
	if (transform)
	{
		Renderer::Instance()->BindUniformBuffer(transform->mMatricesUBO, 1);
	}

	Material* material = object->GetComponent<Material>();
	if (material)
	{
		if (material->pTexture)
		{
			Renderer::Instance()->BindTexture(material->pTexture, 8);
		}
		Renderer::Instance()->BindUniformBuffer(material->mMaterialUBO, 3);
	}

	Shape* shape = object->GetComponent<Shape>();
	if (shape)
	{
		if (shape && Renderer::Instance()->GetCullingFace() != CullFace::None)
		{
			if (shape->mShape == Shapes::QUAD)
			{
				Renderer::Instance()->SetCullingFace(CullFace::Back);
			}
			else
			{
				Renderer::Instance()->SetCullingFace(CullFace::Front);
			}
		}

		if (shape->mMesh)
		{
			Renderer::Instance()->Draw(shape->mShapeData->mVBO, shape->mShapeData->mIBO, shader);
		}
		if (shape->mWireMesh)
		{
			Renderer::Instance()->DebugDrawLines(shape->mShapeData->mVBO, shape->mShapeData->mIBO, shader);
		}
		if (shape->mDebugMesh)
		{
			Renderer::Instance()->DebugDraw(shape->mShapeData->mVBO, shape->mShapeData->mIBO, shader);
		}
	}

	if (transform)
	{
		Renderer::Instance()->UnbindUniformBuffer(transform->mMatricesUBO);
	}

	if (material)
	{
		if (material->pTexture)
		{
			Renderer::Instance()->UnbindTexture(material->pTexture, 8);
		}
		Renderer::Instance()->UnbindUniformBuffer(material->mMaterialUBO);
	}
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
