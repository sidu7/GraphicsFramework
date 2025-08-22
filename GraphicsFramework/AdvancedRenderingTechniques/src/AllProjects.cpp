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
	glm::mat4 mProjection;
	glm::mat4 mView;
	glm::mat4 mInvView;
};

AllProjects::~AllProjects()
{
}

void AllProjects::Init()
{
	baseShader = RenderingFactory::Instance()->CreateShader();
	baseShader->Init("DRAWING");

	lighting = RenderingFactory::Instance()->CreateShader(); 
	lighting->Init("LIGHTING");

	ambient = RenderingFactory::Instance()->CreateShader(); 
	ambient->Init("AMBIENT");

	ambientNoAO = RenderingFactory::Instance()->CreateShader();
	ambientNoAO->Init("AMBIENT_NO_AO");

	shadow = RenderingFactory::Instance()->CreateShader(); 
	shadow->Init("SHADOW");

	locallight = RenderingFactory::Instance()->CreateShader(); 
	locallight->Init("LOCAL_LIGHT");

	globalMatrices = RenderingFactory::Instance()->CreateUniformBuffer();
	globalMatrices->Init(sizeof(Matrices), 2);

	shadowMatrices = RenderingFactory::Instance()->CreateUniformBuffer();
	shadowMatrices->Init(sizeof(Matrices), 2);

	lightDataUbo = RenderingFactory::Instance()->CreateUniformBuffer();
	lightDataUbo->Init(sizeof(LightData), 4);

	lightDynamicDataUbo = RenderingFactory::Instance()->CreateUniformBuffer();
	lightDynamicDataUbo->Init(sizeof(DynamicLightData), 6);
	
	localLightDataUbo = RenderingFactory::Instance()->CreateUniformBuffer();
	localLightDataUbo->Init(sizeof(LocalLightData), 4);

	blurHorizontal = RenderingFactory::Instance()->CreateComputeShader(); 
	blurHorizontal->SetShader("res/shaders/BlurHorizontal.compute");
	blurVertical = RenderingFactory::Instance()->CreateComputeShader(); 
	blurVertical->SetShader("res/shaders/BlurVertical.compute");

	light = new Light(glm::vec3(100.0f, 200.0f, 10.0f));

	Engine::Instance()->GetCamera()->pitch = -15.0f;
	Engine::Instance()->GetCamera()->yaw = -279.3f;
	Engine::Instance()->GetCamera()->mCameraPos = glm::vec3(27.8f, 25.9f, -65.7f);
	Engine::Instance()->GetCamera()->CalculateFront();

	lightData.Light = glm::vec3(1.f);
	lightData.Ambient = glm::vec3(0.1f);

	showAO = true;

	G_Buffer = RenderingFactory::Instance()->CreateFrameBuffer(); 
	G_Buffer->Init(Window::Instance()->GetWidth(), Window::Instance()->GetHeight(), ImageFormat::RGBA32F, 4);
	ShadowMap = RenderingFactory::Instance()->CreateFrameBuffer(); 
	ShadowMap->Init(1024, 1024, ImageFormat::RGBA32F);

	lightData.ShowGBuffer = 0;

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
	block->Init(MAX_BLUR_WEIGHTS_NUM * sizeof(float), 5);

	lightData.BiasAlpha = 0.057f;

	// Shadow blur
	horizontalBlurred = RenderingFactory::Instance()->CreateTexture();
	horizontalBlurred->Init(ImageFormat::RGBA32F, ShadowMap->GetWidth(), ShadowMap->GetHeight());
	blurredShadowMap = RenderingFactory::Instance()->CreateTexture(); 
	blurredShadowMap->Init(ImageFormat::RGBA32F, ShadowMap->GetWidth(), ShadowMap->GetHeight());

	//SkyDome shaders
	skyDomeShader = RenderingFactory::Instance()->CreateShader(); 
	skyDomeShader->Init("SKYDOME");
	skyDomeTexture = RenderingFactory::Instance()->CreateTexture(); 
	skyDomeTexture->Init("res/Textures/popDome.hdr");
	skyDomeIrradiance = RenderingFactory::Instance()->CreateTexture(); 
	skyDomeIrradiance->Init("res/Textures/popDomeIrr.hdr");
	lightData.Exposure = 5.2f;
	lightData.Contrast = 1.0f;

	// IBL
	Hblock.N = HBlockSize;
	HammersleyRandomPoints();
	HUniBlock = RenderingFactory::Instance()->CreateUniformBuffer(); 
	HUniBlock->Init(sizeof(Hblock), 5);
	HUniBlock->AddData(sizeof(Hblock), &Hblock);
	lightData.ShowIBLDiffuse = true;
	lightData.ShowIBLSpecular = true;

	//SSAO
	AOShader = RenderingFactory::Instance()->CreateShader(); 
	AOShader->Init("AMBIENT_OCCLUSION");
	BlurredAO = RenderingFactory::Instance()->CreateFrameBuffer(); 
	BlurredAO->Init(Window::Instance()->GetWidth(), Window::Instance()->GetHeight(), ImageFormat::RGBA32F);
	BilateralHorizontal = RenderingFactory::Instance()->CreateComputeShader(); 
	BilateralHorizontal->SetShader("res/shaders/BilateralHorizontal.compute");
	BilateralVertical = RenderingFactory::Instance()->CreateComputeShader(); 
	BilateralVertical->SetShader("res/shaders/BilateralVertical.compute");
	HorizontalBlurredAO = RenderingFactory::Instance()->CreateTexture(); 
	HorizontalBlurredAO->Init(ImageFormat::RGBA32F, Window::Instance()->GetWidth(), Window::Instance()->GetHeight());
	ResultBlurredAO = RenderingFactory::Instance()->CreateTexture(); 
	ResultBlurredAO->Init(ImageFormat::RGBA32F, Window::Instance()->GetWidth(), Window::Instance()->GetHeight());

	AoParameters.AONum = 115;
	AoParameters.AORadius = 10.0f;
	AoParameters.AOScale = 3.0f;
	AoParameters.AOContrast = 1.0f;

	AoUbo = RenderingFactory::Instance()->CreateUniformBuffer();
	AoUbo->Init(sizeof(AOParams), 3, &AoParameters);

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

	skyDome = ObjectManager::Instance()->ReadObject("res/JSON Data/SkyDome.json");
	skyDome->GetComponent<Transform>()->mModelTransformation = glm::scale(glm::mat4(1.0f), skyDome->GetComponent<Transform>()->mScale);
	showLocalLights = true;

	UpdateLightDataUbo();
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
	delete shadowMatrices;
	delete lightDataUbo;
	delete lightDynamicDataUbo;
	delete localLightDataUbo;
	delete blurHorizontal;
	delete blurVertical;
	delete light;

	delete AoUbo;
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

	Matrices MatData{
		Engine::Instance()->GetCamera()->mProjection,
		Engine::Instance()->GetCamera()->mView ,
		glm::inverse(Engine::Instance()->GetCamera()->mView)
	};
	globalMatrices->AddData(sizeof(MatData), &MatData);
	pRenderer->BindUniformBuffer(globalMatrices, globalMatrices->GetBinding());

	ObjectManager::Instance()->RenderObjects(baseShader);

	pRenderer->UnbindShader(baseShader);
	pRenderer->UnbindFrameBuffer(G_Buffer);
	pRenderer->UnbindUniformBuffer(globalMatrices);

	//ShadowMap pass
	pRenderer->BindFrameBuffer(ShadowMap);
	ShadowMap->Clear();
	pRenderer->BindShader(shadow);

	glm::mat4 LightLookAt, LightProj;
	LightLookAt = glm::lookAt(light->position, glm::vec3(0.0f,0.0f,0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	LightProj = glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 2000.0f);

	Matrices ShadowMatricesData{ LightProj, LightLookAt , glm::inverse(LightLookAt)};
	shadowMatrices->AddData(sizeof(ShadowMatricesData), &ShadowMatricesData);
	pRenderer->BindUniformBuffer(shadowMatrices, shadowMatrices->GetBinding());

	dynamicLightData.ShadowMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.5f)) * glm::scale(glm::mat4(1.0f), glm::vec3(0.5f)) * LightProj * LightLookAt;

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

	pRenderer->SetDepthTest(false);
	pRenderer->SetBlending(false);

	//AO Pass
	pRenderer->BindFrameBuffer(BlurredAO);
	BlurredAO->Clear();
	pRenderer->BindUniformBuffer(AoUbo, AoUbo->GetBinding());
	pRenderer->BindShader(AOShader);
	pRenderer->BindTexture(G_Buffer->GetTexture(0), 2);
	pRenderer->BindTexture(G_Buffer->GetTexture(1), 3);
	Renderer::Instance()->DrawQuad(AOShader);
	pRenderer->UnbindShader(AOShader);
	pRenderer->UnbindUniformBuffer(AoUbo);
	pRenderer->UnbindTexture(G_Buffer->GetTexture(0), 2);
	pRenderer->UnbindTexture(G_Buffer->GetTexture(1), 3);

	pRenderer->UnbindFrameBuffer(BlurredAO);

	//Bilateral Filter Blur
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
	Rect3D Viewport;
	Viewport.Size = glm::vec2(Window::Instance()->GetWidth(), Window::Instance()->GetHeight());
	pRenderer->SetViewportSize(Viewport);
	pRenderer->Clear();

	pRenderer->SetDepthTest(false);
	pRenderer->SetBlending(false);

	Shader* ambientShader = showAO ? ambient : ambientNoAO;
	pRenderer->BindShader(ambientShader);
	pRenderer->BindTexture(G_Buffer->GetTexture(0), 2);
	pRenderer->BindTexture(G_Buffer->GetTexture(1), 3);
	pRenderer->BindTexture(G_Buffer->GetTexture(2), 4);
	pRenderer->BindTexture(G_Buffer->GetTexture(3), 5);
	pRenderer->BindTexture(skyDomeIrradiance, 6);
	pRenderer->BindTexture(skyDomeTexture, 7);

	if (showAO)
	{
		pRenderer->BindTexture(ResultBlurredAO, 8);
	}
	pRenderer->BindUniformBuffer(globalMatrices, globalMatrices->GetBinding());
	pRenderer->BindUniformBuffer(lightDataUbo, lightDataUbo->GetBinding());
	Renderer::Instance()->BindUniformBuffer(HUniBlock, 5);
	Renderer::Instance()->DrawQuad(ambientShader);
	Renderer::Instance()->UnbindUniformBuffer(HUniBlock);
	pRenderer->UnbindUniformBuffer(lightDataUbo);
	pRenderer->UnbindShader(ambientShader);
	pRenderer->UnbindUniformBuffer(globalMatrices);

	pRenderer->UnbindTexture(G_Buffer->GetTexture(0), 2);
	pRenderer->UnbindTexture(G_Buffer->GetTexture(1), 3);
	pRenderer->UnbindTexture(G_Buffer->GetTexture(2), 4);
	pRenderer->UnbindTexture(G_Buffer->GetTexture(3), 5);
	pRenderer->UnbindTexture(skyDomeIrradiance, 6);
	pRenderer->UnbindTexture(skyDomeTexture, 7);
	pRenderer->UnbindTexture(ResultBlurredAO, 8);

	// Global Lighting pass
	if (lightData.ShowGBuffer == 0)
	{
		pRenderer->SetBlending(true);
	}

	if (lighton)
	{
		pRenderer->BindShader(lighting);
		pRenderer->BindTexture(G_Buffer->GetTexture(0), 1);
		pRenderer->BindTexture(G_Buffer->GetTexture(1), 2);
		pRenderer->BindTexture(G_Buffer->GetTexture(2), 3);
		pRenderer->BindTexture(G_Buffer->GetTexture(3), 4);
		if (softShadows)
		{
			pRenderer->BindTexture(blurredShadowMap, 5);
		}
		else
		{
			pRenderer->BindTexture(ShadowMap->GetTexture(), 5);
		}
		dynamicLightData.LightPos = light->position;
		lightDynamicDataUbo->AddData(sizeof(DynamicLightData), &dynamicLightData);
		pRenderer->BindUniformBuffer(lightDynamicDataUbo, lightDynamicDataUbo->GetBinding());
		pRenderer->BindUniformBuffer(globalMatrices, globalMatrices->GetBinding());
		pRenderer->BindUniformBuffer(lightDataUbo, lightDataUbo->GetBinding());
		Renderer::Instance()->DrawQuad(lighting);
		pRenderer->UnbindShader(lighting);
		pRenderer->UnbindTexture(G_Buffer->GetTexture(0), 1);
		pRenderer->UnbindTexture(G_Buffer->GetTexture(1), 2);
		pRenderer->UnbindTexture(G_Buffer->GetTexture(2), 3);
		pRenderer->UnbindTexture(G_Buffer->GetTexture(3), 4);
		pRenderer->UnbindUniformBuffer(lightDynamicDataUbo);
		pRenderer->UnbindUniformBuffer(lightDataUbo);
		pRenderer->UnbindUniformBuffer(globalMatrices);
	}

	// Local Lighting pass
	if (lightData.ShowGBuffer == 0 && showLocalLights)
	{
		pRenderer->SetCullingFace(CullFace::Front);

		pRenderer->BindShader(locallight);
		pRenderer->BindUniformBuffer(globalMatrices, globalMatrices->GetBinding());
		pRenderer->BindTexture(G_Buffer->GetTexture(0), 1);
		pRenderer->BindTexture(G_Buffer->GetTexture(1), 2);
		pRenderer->BindTexture(G_Buffer->GetTexture(2), 3);
		pRenderer->BindTexture(G_Buffer->GetTexture(3), 4);
		
		float lightRadius = 4.f;
		localLightData.LightRadius = lightRadius;
		pRenderer->BindUniformBuffer(localLightDataUbo, localLightDataUbo->GetBinding());

		for (unsigned int i = 0; i < 40; ++i)
		{
			for (unsigned int j = 0; j < 40; ++j)
			{
				localLightData.ModelMatrix = glm::mat4(1.0f);
				localLightData.ModelMatrix = glm::translate(localLightData.ModelMatrix, glm::vec3(i * lightRadius * 2 - 40.0f * lightRadius, lightRadius / 2, j * lightRadius * 2 - 40.0f * lightRadius));
				localLightData.ModelMatrix = glm::scale(localLightData.ModelMatrix, glm::vec3(lightRadius));
				localLightData.Light = lightColors[i][j];
				localLightData.LightPos = glm::vec3(i * lightRadius * 2 - 40.0f * lightRadius, lightRadius / 2, j* lightRadius * 2 - 40.0f * lightRadius);

				localLightDataUbo->AddData(sizeof(localLightData), &localLightData);
				ShapeData& shape = ShapeManager::Instance()->mShapes[Shapes::SPHERE];
				Renderer::Instance()->Draw(shape.mVBO, shape.mIBO, locallight);
			}
		}

		pRenderer->UnbindUniformBuffer(localLightDataUbo);
		pRenderer->UnbindTexture(G_Buffer->GetTexture(0), 1);
		pRenderer->UnbindTexture(G_Buffer->GetTexture(1), 2);
		pRenderer->UnbindTexture(G_Buffer->GetTexture(2), 3);
		pRenderer->UnbindTexture(G_Buffer->GetTexture(3), 4);
	}

	pRenderer->SetCullingFace(CullFace::None);
	pRenderer->SetBlending(false);
	pRenderer->SetDepthTest(true);

	// Copy depth contents from G-Buffer
	G_Buffer->CopyDepthTo(Renderer::Instance()->GetBackBuffer());

	// Forward render Skydome
	if (lightData.ShowGBuffer == 0)
	{
		pRenderer->BindShader(skyDomeShader);

		Transform* skyDomeTr = skyDome->GetComponent<Transform>();
		ObjectMatricesUBO SkyDomeUboData{
			glm::inverse(skyDomeTr->mModelTransformation),
			skyDomeTr->mModelTransformation,
			skyDomeTr->mPrevModelTransformation };
		skyDomeTr->mMatricesUBO->AddData(sizeof(SkyDomeUboData), &SkyDomeUboData);

		pRenderer->BindUniformBuffer(globalMatrices, globalMatrices->GetBinding());
		pRenderer->BindUniformBuffer(skyDomeTr->mMatricesUBO, skyDomeTr->mMatricesUBO->GetBinding());
		pRenderer->BindUniformBuffer(lightDataUbo, lightDataUbo->GetBinding());
		pRenderer->BindTexture(skyDomeTexture, 1);

		Renderer::Instance()->Draw(skyDome->GetComponent<Shape>()->mShapeData->mVBO, skyDome->GetComponent<Shape>()->mShapeData->mIBO, skyDomeShader);
		
		pRenderer->UnbindShader(skyDomeShader);
		pRenderer->UnbindTexture(skyDomeTexture, 1);
		pRenderer->UnbindUniformBuffer(globalMatrices);
		pRenderer->UnbindUniformBuffer(skyDomeTr->mMatricesUBO);
		pRenderer->UnbindUniformBuffer(lightDataUbo);
	}

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
	if (ImGui::BeginCombo("Select G-Buffer", bufferList[lightData.ShowGBuffer]))
	{
		lighton = true;
		for (int i = 0; i < ARRAY_SIZE(bufferList); ++i)
		{
			if (ImGui::Selectable(bufferList[i], selected))
			{
				lightData.ShowGBuffer = i;
				UpdateLightDataUbo();
			}
		}
		ImGui::EndCombo();
	}
	ImGui::DragFloat3("Light Position", &light->position[0], 2.0f, -300.0f, 300.0f);
	if (ImGui::InputFloat("biasAlpha", &lightData.BiasAlpha, 0.0005))
	{
		UpdateLightDataUbo();
	}
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
	if (ImGui::InputFloat("Exposure", &lightData.Exposure, 0.2))
	{
		UpdateLightDataUbo();
	}
	if (ImGui::InputFloat("Contrast", &lightData.Contrast, 0.2))
	{
		UpdateLightDataUbo();
	}
	ImGui::Checkbox("Pause Moving", &Engine::Instance()->stopMoving);
	if (ImGui::Checkbox("Show IBL Diffuse", &lightData.ShowIBLDiffuse))
	{
		UpdateLightDataUbo();
	}
	if (ImGui::Checkbox("Show IBL Specular", &lightData.ShowIBLSpecular))
	{
		UpdateLightDataUbo();
	}
	ImGui::InputInt("AONum", &AoParameters.AONum);
	ImGui::InputFloat("AORadius", &AoParameters.AORadius, 1.0f);
	ImGui::InputFloat("AOScale", &AoParameters.AOScale, 0.1f);
	ImGui::InputFloat("AOContrast", &AoParameters.AOContrast, 0.1f);
	ImGui::End();
}

void AllProjects::RenderObject(Object* object, Shader* shader)
{
	Transform* transform = object->GetComponent<Transform>();
	if (transform && transform->mMatricesUBO)
	{
		Renderer::Instance()->BindUniformBuffer(transform->mMatricesUBO, transform->mMatricesUBO->GetBinding());
	}

	Material* material = object->GetComponent<Material>();
	if (material)
	{
		if (material->pTexture)
		{
			Renderer::Instance()->BindTexture(material->pTexture, 8);
		}
		if (material->mMaterialUBO)
		{
			Renderer::Instance()->BindUniformBuffer(material->mMaterialUBO, material->mMaterialUBO->GetBinding());
		}
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

void AllProjects::UpdateLightDataUbo()
{
	lightDataUbo->AddData(sizeof(LightData), &lightData);
}
