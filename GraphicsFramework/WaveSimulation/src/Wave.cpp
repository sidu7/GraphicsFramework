#include "Wave.h"

#include <core/Engine.h>
#include <core/Camera.h>
#include <core/Light.h>
#include <core/Time.h>
#include <core/Inputs.h>
#include <opengl/Renderer.h>
#include <opengl/Shader.h>
#include <opengl/VertexArray.h>
#include <opengl/VertexBuffer.h>
#include <opengl/ElementArrayBuffer.h>
#include <glm/gtc/matrix_transform.hpp>
#include <Imgui/imgui.h>
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

	Engine::Instance().pCamera->mCameraPos = glm::vec3(0.1f, 2.0f, 2.2f);
	Engine::Instance().pCamera->pitch = -39.0f;
	Engine::Instance().pCamera->yaw = -95.0f;
	Engine::Instance().pCamera->mSpeed /= 4.0f;
	Engine::Instance().pCamera->CalculateFront();

	mShader = new Shader("res/shaders/Drawing.vert", "res/shaders/Drawing.frag");

	mHeightMap.initialize();

	mDiffuseColor = glm::vec3(0.8f, 0.8f, 1);

	mVAO = new VertexArray();
	mVAO->Bind();
	int size = mHeightMap.grid.width() * mHeightMap.grid.height() * sizeof(glm::vec3);
	
	mVBOs[VERT] = new VertexBuffer();
	mVBOs[VERT]->Bind();
	mVBOs[VERT]->AddDynamicData(NULL, size);
	mVAO->Push(3, GL_FLOAT, sizeof(float));
	mVAO->AddLayout();

	mVBOs[NORM] = new VertexBuffer();
	mVBOs[NORM]->Bind();
	mVBOs[NORM]->AddDynamicData(NULL, size);
	mVAO->Push(3, GL_FLOAT, sizeof(float));
	mVAO->AddLayout();

	mEBO = new ElementArrayBuffer();
	mEBO->Bind();
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
	mVAO->Unbind();

	mMeshPositions.resize(mHeightMap.grid.width() * mHeightMap.grid.height());
	mMeshNormals.resize(mHeightMap.grid.width() * mHeightMap.grid.height());

	glEnable(GL_DEPTH_TEST);
	Renderer::Instance().SetClearColor(glm::vec3(1.0f, 1.0f, 1.0f));
}

void Wave::Update()
{
	glClearDepth(1);
	Renderer::Instance().Clear();

	mShader->Bind();
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

	mShader->SetUniformMat4f("vp", Engine::Instance().pCamera->mProjection * Engine::Instance().pCamera->mView);

	// update mesh positions and normals on GPU
	for (int j = 0; j < mHeightMap.grid.height(); ++j)
		for (int i = 0; i < mHeightMap.grid.width(); ++i) {
			int index = mHeightMap.grid.width() * j + i;
			mMeshPositions[index] = glm::vec3(i, j, mHeightMap.grid(i, j).real());
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

	Renderer::Instance().Draw(*mVAO, *mEBO, *mShader);

	if(Inputs::Instance().IsMouseClicked(SDL_BUTTON_LEFT))
	{
		glm::vec2 mouse_pos = Inputs::Instance().GetMouseLocation();
		mouse_pos /= Engine::Instance().GetWindowSize();
		mHeightMap.handle_input(mouse_pos);
	}

	mHeightMap.update(Time::Instance().deltaTime);
}

void Wave::DebugDisplay()
{
	if(ImGui::Button("Reset"))
	{
		mHeightMap.initialize();
	}
}
