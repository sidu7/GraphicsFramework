#include "Wave.h"

#include "Core/Engine.h"
#include "Core/Camera.h"
#include "Core/Light.h"
#include "Core/Time.h"
#include "Core/Inputs.h"
#include "Core/Window.h"
#include "Rendering/Renderer.h"
#include "Rendering/RenderingFactory.h"
#include "Rendering/Shader.h"
#include "Rendering/VertexArray.h"
#include "Rendering/VertexBuffer.h"
#include "Rendering/ElementArrayBuffer.h"
#include "glm/gtc/matrix_transform.hpp"
#include "Imgui/imgui.h"
#include <iostream>

Wave::~Wave()
{
	delete mLight;
	delete mShader;
	delete mVBOs[VERT]; delete mVBOs[NORM];
	delete mEBO;
	delete mVAO;
}

void Wave::Init()
{
	mLight = new Light(glm::vec3(1.0, -0.3, -0.1));

	Engine::Instance()->GetCamera()->mCameraPos = glm::vec3(0.1f, 2.0f, 2.2f);
	Engine::Instance()->GetCamera()->pitch = -39.0f;
	Engine::Instance()->GetCamera()->yaw = -95.0f;
	Engine::Instance()->GetCamera()->mSpeed /= 4.0f;
	Engine::Instance()->GetCamera()->CalculateFront();

	mShader = RenderingFactory::Instance()->CreateShader();
	mShader->Init("res/shaders/Drawing.vert", "res/shaders/Drawing.frag");

	mHeightMap.initialize();

	mDiffuseColor = glm::vec3(0.8f, 0.8f, 1);

	mVAO = RenderingFactory::Instance()->CreateVertexArray();
	Renderer::Instance()->BindVertexArray(mVAO);
	int size = mHeightMap.grid.width() * mHeightMap.grid.height() * sizeof(glm::vec3);
	
	mVBOs[VERT] = RenderingFactory::Instance()->CreateVertexBuffer();
	Renderer::Instance()->BindVertexBuffer(mVBOs[VERT]);
	mVBOs[VERT]->AddDynamicData(NULL, size);
	mVAO->Push(3, GL_FLOAT, sizeof(float));
	mVAO->AddLayout();

	mVBOs[NORM] = RenderingFactory::Instance()->CreateVertexBuffer();
	Renderer::Instance()->BindVertexBuffer(mVBOs[NORM]);
	mVBOs[NORM]->AddDynamicData(NULL, size);
	mVAO->Push(3, GL_FLOAT, sizeof(float));
	mVAO->AddLayout();

	mEBO = RenderingFactory::Instance()->CreateElementArrayBuffer();
	Renderer::Instance()->BindElementArrayBuffer(mEBO);
	mTriCount = 2 * (mHeightMap.grid.width() - 1) * (mHeightMap.grid.height() - 1);
	unsigned* faces = new unsigned[3 * mTriCount];
	for (int j = 1, n = 0; j < mHeightMap.grid.height(); ++j) {
		for (int i = 1; i < mHeightMap.grid.width(); ++i, n += 6) {
			faces[n + 0] = mHeightMap.grid.width() * (j - 1) + (i - 1);
			faces[n + 1] = mHeightMap.grid.width() * (j - 1) + i;
			faces[n + 2] = mHeightMap.grid.width() * j + i;
			faces[n + 3] = faces[n + 0];
			faces[n + 4] = faces[n + 2];
			faces[n + 5] = mHeightMap.grid.width() * j + i - 1;
		}
	}
	mEBO->AddData(faces, 3 * mTriCount, sizeof(unsigned));
	delete[] faces;
	Renderer::Instance()->UnbindVertexArray(mVAO);

	mMeshPositions.resize(mHeightMap.grid.width() * mHeightMap.grid.height());
	mMeshNormals.resize(mHeightMap.grid.width() * mHeightMap.grid.height());

	glEnable(GL_DEPTH_TEST);
	Renderer::Instance()->SetClearColor(glm::vec3(1.0f, 1.0f, 1.0f));

	mWaveIntensity = 0.01f;
}

void Wave::Update()
{
	glClearDepth(1);
	Renderer::Instance()->Clear();

	Renderer::Instance()->BindShader(mShader);
	mShader->SetUniform3f("light_position", mLight->position);
	mShader->SetUniform3f("diffuse_color", mDiffuseColor);
	
	glm::mat4 I = glm::mat4(1);
	glm::mat4 model_trans = glm::rotate(I, glm::radians(-90.0f),glm::vec3(1.0f,0.0f,0.0f)) * glm::translate(I, glm::vec3(-1, -1, 0))
		* glm::scale(I, glm::vec3(2.0f / mHeightMap.grid.width(),
			2.0f / mHeightMap.grid.height(),
			1.0f));
	mShader->SetUniformMat4f("model_trans", model_trans);

	glm::mat4 normal_trans = glm::scale(I, glm::vec3(0.5f * mHeightMap.grid.width(),
			0.5f * mHeightMap.grid.height(),
			1.0f));
	mShader->SetUniformMat4f("normal_trans", glm::inverse(model_trans));

	mShader->SetUniformMat4f("vp", Engine::Instance()->GetCamera()->mProjection * Engine::Instance()->GetCamera()->mView);

	// update mesh positions and normals on GPU
	for (int j = 0; j < mHeightMap.grid.height(); ++j)
		for (int i = 0; i < mHeightMap.grid.width(); ++i) {
			int index = mHeightMap.grid.width() * j + i;
			mMeshPositions[index] = glm::vec3(i, j, glm::clamp(mHeightMap.grid(i, j).real(), 0.f, mHeightMap.max_height * mWaveIntensity));
			float dx = 0,
				dy = 0;
			if (i > 0 && i + 1 < mHeightMap.grid.width())
				dx = 0.5f * (mHeightMap.grid(i + 1, j).real() - mHeightMap.grid(i - 1, j)).real();
			else if (i > 0)
				dx = mHeightMap.grid(i, j).real() - mHeightMap.grid(i - 1, j).real();
			else
				dx = mHeightMap.grid(i + 1, j).real() - mHeightMap.grid(i, j).real();
			if (j > 0 && j + 1 < mHeightMap.grid.height())
				dy = 0.5f * (mHeightMap.grid(i, j + 1).real() - mHeightMap.grid(i, j - 1)).real();
			else if (j > 0)
				dy = mHeightMap.grid(i, j).real() - mHeightMap.grid(i, j - 1).real();
			else
				dy = mHeightMap.grid(i, j + 1).real() - mHeightMap.grid(i, j).real();
			mMeshNormals[index] = glm::vec3(-dx, -dy, 1);
		}

	int size = mHeightMap.grid.width() * mHeightMap.grid.height();

	mVBOs[VERT]->AddSubData(&mMeshPositions[0], size * sizeof(glm::vec3));
	mVBOs[NORM]->AddSubData(&mMeshNormals[0], size * sizeof(glm::vec3));

	Renderer::Instance()->Draw(mVAO, mEBO, mShader);

	if(Inputs::Instance()->IsMouseClicked(SDL_BUTTON_LEFT))
	{
		glm::vec2 mouse_pos = Inputs::Instance()->GetMouseLocation();
		mouse_pos /= Window::Instance()->GetWindowSize();
		mHeightMap.handle_input(mouse_pos);
	}

	mHeightMap.update(Time::Instance()->deltaTime);
}

void Wave::DebugDisplay()
{
	if(ImGui::Button("Reset"))
	{
		mHeightMap.initialize();
	}
	ImGui::SliderFloat("Wave Intensity", &mWaveIntensity, 0.01f, 1.f);
}
