#include "MotionBlur.h"

#include "Core/Engine.h"
#include "Core/Window.h"
#include "Core/Camera.h"
#include "Core/Light.h"
#include "Core/ObjectManager.h"
#include "Core/Time.h"
#include "Core/Object.h"
#include "Core/Components/Transform.h"
#include "Core/Components/Shape.h"
#include "Core/Components/Material.h"
#include "Core/ShapeManager.h"
#include "Rendering/RenderingFactory.h"
#include "Rendering/Shader.h"
#include "Rendering/FrameBuffer.h"
#include "Rendering/UniformBuffer.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <Imgui/imgui.h>

struct Matrices
{
	glm::mat4 mView;
	glm::mat4 mProjection;
	glm::mat4 mPrevView;
};

MotionBlur::~MotionBlur()
{
}

void MotionBlur::Init()
{
	baseShader = RenderingFactory::Instance()->CreateShader();
	baseShader->Init("GBUFFER");

	lighting = RenderingFactory::Instance()->CreateShader(); 
	lighting->Init("MOTIONBLUR_LIGHTING");

	ambient = RenderingFactory::Instance()->CreateShader(); 
	ambient->Init("MOTIONBLUR_AMBIENT");

	light = new Light(glm::vec3(100.0f, 200.0f, 10.0f));

	Engine::Instance()->GetCamera()->pitch = -20.0f;
	Engine::Instance()->GetCamera()->yaw = -265.3f;
	Engine::Instance()->GetCamera()->mCameraPos = glm::vec3(51.0f, 92.0f, -240.0f);
	Engine::Instance()->GetCamera()->mSpeed = 60.0f;
	Engine::Instance()->GetCamera()->CalculateFront();

	Renderer::Instance()->BindShader(lighting);
	//lighting->SetUniform3f("Light", 3.2f, 3.2f, 3.2f);

	Renderer::Instance()->BindShader(ambient);
	//ambient->SetUniform3f("Ambient", 0.1f, 0.1f, 0.1f);

	G_Buffer = RenderingFactory::Instance()->CreateFrameBuffer(); 
	G_Buffer->Init(Window::Instance()->GetWidth(), Window::Instance()->GetHeight(), ImageFormat::RGBA32F, 5);

	Color = RenderingFactory::Instance()->CreateFrameBuffer(); 
	Color->Init(Window::Instance()->GetWidth(), Window::Instance()->GetHeight(), ImageFormat::RGBA32F);

	gBuffershow = 0;

	globalMatrices = RenderingFactory::Instance()->CreateUniformBuffer();
	globalMatrices->Init(sizeof(Matrices), 2);

	//SkyDome shaders
	skyDomeShader = RenderingFactory::Instance()->CreateShader(); 
	skyDomeShader->Init("SKYDOME");

	skyDomeTexture = RenderingFactory::Instance()->CreateTexture(); 
	skyDomeTexture->Init("res/Textures/popDome.hdr");
	exposure = 5.2f;
	contrast = 1.0f;

	// Load Objects in Scene
	ObjectManager::Instance()->AddObject("res/JSON Data/Floor.json");
	//ObjectManager::Instance().AddObject("res/JSON Data/UpDown Teapot1.json");
	//ObjectManager::Instance().AddObject("res/JSON Data/UpDown Teapot2.json");
	ObjectManager::Instance()->AddObject("res/JSON Data/LeftRight Teapot1.json");
	ObjectManager::Instance()->AddObject("res/JSON Data/LeftRight Teapot2.json");
	ObjectManager::Instance()->AddObject("res/JSON Data/Circle Teapot1.json");
	ObjectManager::Instance()->AddObject("res/JSON Data/Circle Teapot2.json");
	//ObjectManager::Instance().AddObject("res/JSON Data/Teapot9.json");
	skyDome = ObjectManager::Instance()->ReadObject("res/JSON Data/SkyDome.json");
	skyDome->GetComponent<Transform>()->mModelTransformation = glm::scale(glm::mat4(1.0f), skyDome->GetComponent<Transform>()->mScale);

	k = 20;
	S = 15;
	TileMax = RenderingFactory::Instance()->CreateFrameBuffer(); 
	TileMax->Init(Window::Instance()->GetWidth() / k, Window::Instance()->GetHeight() / k, ImageFormat::RGBA32F);
	NeighbourMax = RenderingFactory::Instance()->CreateFrameBuffer(); 
	NeighbourMax->Init(Window::Instance()->GetWidth() / k, Window::Instance()->GetHeight() / k, ImageFormat::RGBA32F);

	TileMaxShader = RenderingFactory::Instance()->CreateShader(); 
	TileMaxShader->Init("TILE_MAX");
	NeighbourMaxShader = RenderingFactory::Instance()->CreateShader(); 
	NeighbourMaxShader->Init("NEIGHBOUR_MAX");

	ReconstructionShader = RenderingFactory::Instance()->CreateShader(); 
	ReconstructionShader->Init("RECONSTRUCTION");
	MotionBlurOn = true;
	MotionBlurShader = RenderingFactory::Instance()->CreateShader(); 
	MotionBlurShader->Init("MOTIONBLUR");
	ReconBlur = false;
	PerPixel = true;

	Engine::Instance()->stopMoving = false;
}

void MotionBlur::Close()
{
	delete baseShader;
	delete lighting;
	delete ambient;
	delete light;

	delete G_Buffer;
	delete Color;
	delete globalMatrices;

	//SkyDome shaders
	delete skyDomeShader;
	delete skyDomeTexture;

	// Clear Objects in Scene
	ObjectManager::Instance()->ClearObjects();

	delete TileMax;
	delete NeighbourMax;
	delete TileMaxShader;
	delete NeighbourMaxShader;
	delete ReconstructionShader;
	delete MotionBlurShader;
}

void MotionBlur::Update()
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

	Matrices MatData{ Engine::Instance()->GetCamera()->mView, Engine::Instance()->GetCamera()->mProjection, Engine::Instance()->GetCamera()->mPrevView };
	globalMatrices->AddData(sizeof(MatData), &MatData);
	//baseShader->SetUniform1f("deltaTime", Time::Instance()->deltaTime);
	//baseShader->SetUniform1i("k", k);

	pRenderer->BindUniformBuffer(globalMatrices, 0);
	ObjectManager::Instance()->RenderObjects(baseShader);

	pRenderer->UnbindUniformBuffer(globalMatrices);
	pRenderer->UnbindShader(baseShader);
	pRenderer->UnbindFrameBuffer(G_Buffer);

	//Ambient light pass

	if (MotionBlurOn)
	{
		pRenderer->BindFrameBuffer(Color);
		Color->Clear();
	}
	else
	{
		Rect3D Viewport;
		Viewport.Size = glm::vec2(Window::Instance()->GetWidth(), Window::Instance()->GetHeight());
		pRenderer->SetViewportSize(Viewport);
		pRenderer->Clear();
	}

	pRenderer->SetDepthTest(false);
	pRenderer->SetBlending(false);

	pRenderer->BindShader(ambient);
	pRenderer->BindTexture(G_Buffer->GetTexture(2), 3);
	//ambient->SetUniform1i("diffusetex", 3);
	Renderer::Instance()->DrawQuad(ambient);
	pRenderer->UnbindShader(ambient);

	pRenderer->SetBlending(true);
	// Global Lighting pass	
	pRenderer->BindShader(lighting);
	pRenderer->BindTexture(G_Buffer->GetTexture(0), 1);
	//lighting->SetUniform1i("normaltex", 1);
	pRenderer->BindTexture(G_Buffer->GetTexture(1), 2);
	//lighting->SetUniform1i("worldpostex", 2);
	pRenderer->BindTexture(G_Buffer->GetTexture(2), 3);
	//lighting->SetUniform1i("diffusetex", 3);
	pRenderer->BindTexture(G_Buffer->GetTexture(3), 4);
	//lighting->SetUniform1i("specularalpha", 4);
	//lighting->SetUniform1f("exposure", exposure);
	//lighting->SetUniform1f("contrast", contrast);
	
	//lighting->SetUniform3f("lightPos", light->position.x, light->position.y, light->position.z);
	//lighting->SetUniformMat4f("inverseview", glm::inverse(Engine::Instance()->GetCamera()->mView));
	//lighting->SetUniform1i("GBufferShow", gBuffershow);
	Renderer::Instance()->DrawQuad(lighting);
	pRenderer->UnbindShader(lighting);

	pRenderer->SetCullingFace(CullFace::None);
	pRenderer->SetBlending(false);
	pRenderer->SetDepthTest(true);

	// Copy depth contents from G-Buffer
	if (MotionBlurOn)
	{
		G_Buffer->CopyDepthTo(Color);
		pRenderer->BindFrameBuffer(Color);
	}
	else
	{
		G_Buffer->CopyDepthTo(Renderer::Instance()->GetBackBuffer());
		pRenderer->BindFrameBuffer(pRenderer->GetBackBuffer());
	}

	// Forward render Skydome
	pRenderer->BindShader(skyDomeShader);
	//skyDomeShader->SetUniformMat4f("view", Engine::Instance()->GetCamera()->mView);
	//skyDomeShader->SetUniformMat4f("projection", Engine::Instance()->GetCamera()->mProjection);
	//skyDomeShader->SetUniformMat4f("model", skyDome->GetComponent<Transform>()->mModelTransformation);
	pRenderer->BindTexture(skyDomeTexture, 1);
	//skyDomeShader->SetUniform1i("skyDome", 1);
	//skyDomeShader->SetUniform1f("exposure", exposure);
	//skyDomeShader->SetUniform1f("contrast", contrast);
	Renderer::Instance()->Draw(skyDome->GetComponent<Shape>()->mShapeData->mVBO, skyDome->GetComponent<Shape>()->mShapeData->mIBO, skyDomeShader);
	pRenderer->UnbindShader(skyDomeShader);
	//Engine::Instance().stopMoving = true;

	if (MotionBlurOn)
	{
		pRenderer->UnbindFrameBuffer(Color);
		pRenderer->SetBlending(false);
		pRenderer->SetDepthTest(false);

		pRenderer->BindShader(MotionBlurShader);
		pRenderer->BindTexture(Color->GetTexture(0), 1);
		//MotionBlurShader->SetUniform1i("Color", 1);
		pRenderer->BindTexture(G_Buffer->GetTexture(4), 2);
		//MotionBlurShader->SetUniform1i("Velocity", 2);
		//MotionBlurShader->SetUniform1i("S", S);
		Renderer::Instance()->DrawQuad(MotionBlurShader);

		pRenderer->UnbindShader(MotionBlurShader);
	}
}

void MotionBlur::DebugDisplay()
{
	ImGui::Begin("G-Buffer");
	ImGui::InputFloat("Exposure", &exposure, 0.2);
	ImGui::InputFloat("Contrast", &contrast, 0.2);
	ImGui::Checkbox("Pause", &Engine::Instance()->stopMoving);
	ImGui::InputInt("Samples", &S);
	ImGui::Checkbox("MotionBlur", &MotionBlurOn);
	ImGui::End();
}

void MotionBlur::RenderObject(Object* object, Shader* shader)
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