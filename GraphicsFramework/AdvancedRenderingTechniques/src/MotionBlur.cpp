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

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <Imgui/imgui.h>

MotionBlur::~MotionBlur()
{
}

void MotionBlur::Init()
{
	baseShader = RenderingFactory::Instance()->CreateShader();
	baseShader->Init("res/motionblur/GBuffer.vert", "res/motionblur/GBuffer.frag");

	lighting = RenderingFactory::Instance()->CreateShader(); 
	lighting->Init("res/motionblur/Lighting.vert", "res/motionblur/Lighting.frag");

	ambient = RenderingFactory::Instance()->CreateShader(); 
	ambient->Init("res/motionblur/Ambient.vert", "res/motionblur/Ambient.frag");

	light = new Light(glm::vec3(100.0f, 200.0f, 10.0f));

	Engine::Instance()->GetCamera()->pitch = -20.0f;
	Engine::Instance()->GetCamera()->yaw = -265.3f;
	Engine::Instance()->GetCamera()->mCameraPos = glm::vec3(51.0f, 92.0f, -240.0f);
	Engine::Instance()->GetCamera()->mSpeed *= 3;
	Engine::Instance()->GetCamera()->CalculateFront();

	Renderer::Instance()->BindShader(lighting);
	lighting->SetUniform3f("Light", 3.2f, 3.2f, 3.2f);

	Renderer::Instance()->BindShader(ambient);
	ambient->SetUniform3f("Ambient", 0.1f, 0.1f, 0.1f);

	G_Buffer = RenderingFactory::Instance()->CreateFrameBuffer(); 
	G_Buffer->Init(Window::Instance()->GetWidth(), Window::Instance()->GetHeight(), 5);

	Color = RenderingFactory::Instance()->CreateFrameBuffer(); 
	Color->Init(Window::Instance()->GetWidth(), Window::Instance()->GetHeight());

	gBuffershow = 0;

	//SkyDome shaders
	skyDomeShader = RenderingFactory::Instance()->CreateShader(); 
	skyDomeShader->Init("res/shaders/SkyDome.vert", "res/shaders/SkyDome.frag");

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
	TileMax->Init(Window::Instance()->GetWidth() / k, Window::Instance()->GetHeight() / k);
	NeighbourMax = RenderingFactory::Instance()->CreateFrameBuffer(); 
	NeighbourMax->Init(Window::Instance()->GetWidth() / k, Window::Instance()->GetHeight() / k);

	TileMaxShader = RenderingFactory::Instance()->CreateShader(); 
	TileMaxShader->Init("res/motionblur/TileMax.vert", "res/motionblur/TileMax.frag");
	NeighbourMaxShader = RenderingFactory::Instance()->CreateShader(); 
	NeighbourMaxShader->Init("res/motionblur/NeighbourMax.vert", "res/motionblur/NeighbourMax.frag");

	ReconstructionShader = RenderingFactory::Instance()->CreateShader(); 
	ReconstructionShader->Init("res/motionblur/Reconstruction.vert", "res/motionblur/Reconstruction.frag");
	MotionBlurOn = true;
	MotionBlurShader = RenderingFactory::Instance()->CreateShader(); 
	MotionBlurShader->Init("res/motionblur/MotionBlur.vert", "res/motionblur/MotionBlur.frag");
	ReconBlur = false;
	PerPixel = true;
}

void MotionBlur::Update()
{
	light->HandleInputs();

	ObjectManager::Instance()->UpdateObjects();

	Renderer* pRenderer = Renderer::Instance();

	//G-Buffer pass
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
	pRenderer->BindFrameBuffer(G_Buffer);
	G_Buffer->Clear();
	pRenderer->BindShader(baseShader);
	baseShader->SetUniformMat4f("view", Engine::Instance()->GetCamera()->mView);
	baseShader->SetUniformMat4f("prevview", Engine::Instance()->GetCamera()->mPrevView);
	baseShader->SetUniformMat4f("projection", Engine::Instance()->GetCamera()->mProjection);
	baseShader->SetUniform1f("deltaTime", Time::Instance()->deltaTime);
	baseShader->SetUniform1i("k", k);

	ObjectManager::Instance()->RenderObjects(baseShader);

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
		glViewport(0, 0, Window::Instance()->GetWidth(), Window::Instance()->GetHeight());
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);

	pRenderer->BindShader(ambient);
	G_Buffer->TexBind(2, 3);
	ambient->SetUniform1i("diffusetex", 3);
	Renderer::Instance()->DrawQuad();
	pRenderer->UnbindShader(ambient);

	glEnable(GL_BLEND);
	// Global Lighting pass	
	pRenderer->BindShader(lighting);
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
	lighting->SetUniformMat4f("inverseview", glm::inverse(Engine::Instance()->GetCamera()->mView));
	lighting->SetUniform1i("GBufferShow", gBuffershow);
	Renderer::Instance()->DrawQuad();
	pRenderer->UnbindShader(lighting);

	glDisable(GL_CULL_FACE);
	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);

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
	skyDomeShader->SetUniformMat4f("view", Engine::Instance()->GetCamera()->mView);
	skyDomeShader->SetUniformMat4f("projection", Engine::Instance()->GetCamera()->mProjection);
	skyDomeShader->SetUniformMat4f("model", skyDome->GetComponent<Transform>()->mModelTransformation);
	pRenderer->BindTexture(skyDomeTexture, 1);
	skyDomeShader->SetUniform1i("skyDome", 1);
	skyDomeShader->SetUniform1f("exposure", exposure);
	skyDomeShader->SetUniform1f("contrast", contrast);
	Renderer::Instance()->Draw(skyDome->GetComponent<Shape>()->mShapeData->m_VAO, skyDome->GetComponent<Shape>()->mShapeData->m_EBO, skyDomeShader);
	pRenderer->UnbindShader(skyDomeShader);
	//Engine::Instance().stopMoving = true;

	if (MotionBlurOn)
	{
		pRenderer->UnbindFrameBuffer(Color);
		glDisable(GL_BLEND);
		glDisable(GL_DEPTH_TEST);


		pRenderer->BindShader(MotionBlurShader);
		Color->TexBind(0, 1);
		MotionBlurShader->SetUniform1i("Color", 1);
		G_Buffer->TexBind(4, 2);
		MotionBlurShader->SetUniform1i("Velocity", 2);
		MotionBlurShader->SetUniform1i("S", S);
		Renderer::Instance()->DrawQuad();

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
			Renderer::Instance()->BindUniformBuffer(material->mMaterialUBO, 3);
		}
	}

	Shape* shape = object->GetComponent<Shape>();
	if (shape)
	{
		if (shape && glIsEnabled(GL_CULL_FACE))
		{
			if (shape->mShape == Shapes::QUAD)
			{
				glCullFace(GL_BACK);
			}
			else
			{
				glCullFace(GL_FRONT);
			}
		}

		if (shape->mMesh)
		{
			Renderer::Instance()->Draw(shape->mShapeData->m_VAO, shape->mShapeData->m_EBO, shader);
		}
		if (shape->mWireMesh)
		{
			Renderer::Instance()->DebugDrawLines(shape->mShapeData->m_VAO, shape->mShapeData->m_EBO, shader);
		}
		if (shape->mDebugMesh)
		{
			Renderer::Instance()->DebugDraw(shape->mShapeData->m_VAO, shape->mShapeData->m_EBO, shader);
		}
	}

	if (material)
	{
		if (material->pTexture)
		{
			Renderer::Instance()->UnbindTexture(material->pTexture, 8);
			Renderer::Instance()->UnbindUniformBuffer(material->mMaterialUBO);
		}
	}
}