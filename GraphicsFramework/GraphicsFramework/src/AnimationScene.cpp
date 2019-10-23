#include "AnimationScene.h"
#include "Engine.h"
#include "Camera.h"
#include "glm/gtc/matrix_transform.hpp"
#include "ShapeManager.h"
#include <math.h>
#include "Time.h"
#include "ImguiManager.h"
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/compatibility.hpp"

extern Engine* engine;

std::string animation;
bool drawModel, drawBones;

void AnimationScene::Init()
{
	demoModel.LoadModel("res/ArcherWalking.fbx");
	demoModel.AddAnimation("res/ArcherDancing.fbx");
	demoModel.AddAnimation("res/ArcherShooting.fbx");
	animation = demoModel.mAnimations[0];
	//demoModel.LoadModel("res/arissa.fbx");

	drawModel = true;
	drawBones = false;

	engine->pCamera->pitch = -20.0f;
	engine->pCamera->mCameraPos = glm::vec3(0.0f, 30.0f, 30.0f);
	engine->pCamera->CalculateFront();

	mBonesTransformation.resize(demoModel.mBones.size());

	light = new Light();
	light->position = glm::vec3(10.0, 100.0, 40.0);

	modelDraw = new Shader("src/animshaders/ModelDraw.vert", "src/animshaders/ModelDraw.frag");
	Drawing = new Shader("src/animshaders/Drawing.vert", "src/animshaders/Drawing.frag");
}

void AnimationScene::Draw()
{
	modelDraw->Bind();

	if (!isPaused)
	{
		RunTime += Time::Instance().deltaTime /2.0f;
	}

	glEnable(GL_DEPTH_TEST);

	ImGui::Begin("Animation");
	bool selected = true;
	if (ImGui::BeginCombo("", "Select Animation"))
	{
		for (unsigned int i = 0; i < demoModel.mAnimations.size(); ++i)
		{
			if (ImGui::Selectable(demoModel.mAnimations[i].c_str(), selected))
			{
				animation = demoModel.mAnimations[i];
				RunTime = 0.01f;
			}
		}
		ImGui::EndCombo();
	}
	ImGui::Checkbox("Draw Skin", &drawModel);
	ImGui::Checkbox("Draw Bones", &drawBones);
	if (ImGui::Button("Step --"))
	{
		isPaused = true;
		RunTime -= 2.0f * Time::Instance().deltaTime;
	}
	ImGui::SameLine();
	if (ImGui::Button("Step -"))
	{
		isPaused = true;
		RunTime -= Time::Instance().deltaTime;
	}
	ImGui::SameLine();
	if (ImGui::Button(isPaused ? "Start" : "Pause"))
	{
		isPaused = !isPaused;
	}
	ImGui::SameLine();
	if (ImGui::Button("Step +"))
	{
		isPaused = true;
		RunTime += Time::Instance().deltaTime;
	}
	ImGui::SameLine();
	if (ImGui::Button("Step ++"))
	{
		isPaused = true;
		RunTime += 2.0f * Time::Instance().deltaTime;
	}
	ImGui::End();

	AnimatorUpdate(animation);

	modelDraw->SetUniform3f("lightPos", light->position.x, light->position.y, light->position.z);
	modelDraw->SetUniform3f("light", 3.2f, 3.2f, 3.2f);
	modelDraw->SetUniformMat4f("inverseview", glm::inverse(engine->pCamera->mView));
	modelDraw->SetUniformMat4f("projection", engine->pCamera->mProjection);
	modelDraw->SetUniformMat4f("view", engine->pCamera->mView);

	glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 7.5f, 0.0f));
	model = glm::scale(model, glm::vec3(0.1f));

	if (drawModel)
	{
		modelDraw->SetUniformMat4f("model", model);
		modelDraw->SetUniformMat4f("normaltr", glm::inverse(model));
		modelDraw->SetUniform3f("diffuse", 1.0f, 0.0f, 0.0f);
		modelDraw->SetUniform3f("specular", 1.0f, 1.0f, 1.0f);
		modelDraw->SetUniform1f("shininess", 0.1f);

		demoModel.Draw(modelDraw);
	}
	

	Drawing->Bind();
	Drawing->SetUniform3f("lightPos", light->position.x, light->position.y, light->position.z);
	Drawing->SetUniform3f("light", 3.2f, 3.2f, 3.2f);
	Drawing->SetUniformMat4f("inverseview", glm::inverse(engine->pCamera->mView));
	Drawing->SetUniformMat4f("projection", engine->pCamera->mProjection);
	Drawing->SetUniformMat4f("view", engine->pCamera->mView);

	if (drawBones)
	{
		if (drawModel)
		{
			glDisable(GL_DEPTH_TEST);
		}
		Drawing->SetUniform1i("lighting", 0);
		for (unsigned int i = 0; i < demoModel.mBones.size(); ++i)
		{
			Drawing->SetUniformMat4f("model", model * demoModel.mBones[i].mCurrentTransformation);
			Drawing->SetUniform3f("diffuse", 1.0f, 0.0f, 0.0f);
			std::pair<VertexArray*, ElementArrayBuffer*> shape = ShapeManager::Instance().mShapes[Shapes::CUBE];
			Renderer::Instance().Draw(*shape.first, *shape.second, *Drawing);
		}
		VertexArray VBO = CreateBonesData();
		glDrawArrays(GL_LINES, 0, demoModel.mBones.size() * 2 - 2);
		Drawing->SetUniform1i("lighting", 1);
	}

	glEnable(GL_DEPTH_TEST);

	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
	model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0, 0.0, 0.0));
	model = glm::scale(model, glm::vec3(500.0f));
	Drawing->SetUniformMat4f("model", model);
	Drawing->SetUniformMat4f("normaltr", glm::inverse(model));
	Drawing->SetUniform3f("diffuse", 0.0f, 1.0f, 0.0f);
	Drawing->SetUniform3f("specular", 0.2f, 0.2f, 0.2f);
	Drawing->SetUniform1f("shininess", 1.0f);
	std::pair<VertexArray*, ElementArrayBuffer*> shape = ShapeManager::Instance().mShapes[Shapes::QUAD];
	Renderer::Instance().Draw(*shape.first, *shape.second, *Drawing);
}

void AnimationScene::AnimatorUpdate(std::string animName)
{
	mBonesTransformation.clear();

	for (unsigned int i = 0; i < demoModel.mBones.size(); ++i)
	{
		Bone& bone = demoModel.mBones[i];
		glm::mat4 trans = glm::mat4(1.0f);
		if (bone.isAnimated)
		{
			//interpolate for timeframe
			AnimationData& anim = bone.mAnimations[animName];
			double TimeinTicks = RunTime * anim.mTicksPerSec;
			double timeFrame = fmod(TimeinTicks, anim.mDuration);
			unsigned int posIndex = FindLessThaninList<glm::vec3>(timeFrame, anim.mKeyPositions);
			unsigned int rotIndex = FindLessThaninList<Quaternion>(timeFrame, anim.mKeyRotations);
			unsigned int sclIndex = FindLessThaninList<glm::vec3>(timeFrame, anim.mKeyScalings);

			glm::vec3 position, scale;
			Quaternion rotation;
			glm::mat4 rot = glm::mat4(1.0f);
			if (posIndex != 0)
			{
				double T2 = anim.mKeyPositions[posIndex].first;
				double T1 = anim.mKeyPositions[posIndex - 1].first;
				double localT = (timeFrame - T1) / (T2 - T1);
				position = glm::lerp(anim.mKeyPositions[posIndex - 1].second, anim.mKeyPositions[posIndex].second, (float)localT);
			}
			else
			{
				position = anim.mKeyPositions[0].second;
			}
			if (rotIndex != 0)
			{
				double T2 = anim.mKeyRotations[rotIndex].first;
				double T1 = anim.mKeyRotations[rotIndex - 1].first;
				double localT = (timeFrame - T1) / (T2 - T1);
				rotation = Quaternion::interpolate(anim.mKeyRotations[rotIndex - 1].second, anim.mKeyRotations[rotIndex].second, localT);
				rot = rotation.matrix();
			}
			else
			{
				rotation = anim.mKeyRotations[0].second;
			}
			if (sclIndex != 0)
			{
				double T2 = anim.mKeyScalings[sclIndex].first;
				double T1 = anim.mKeyScalings[sclIndex - 1].first;
				double localT = (timeFrame - T1) / (T2 - T1);
				scale = glm::lerp(anim.mKeyScalings[sclIndex - 1].second, anim.mKeyScalings[sclIndex].second, (float)localT);
			}
			else
			{
				scale = anim.mKeyScalings[0].second;
			}
			glm::mat4 translate = glm::translate(glm::mat4(1.0f), position);
			glm::mat4 scaling = glm::scale(glm::mat4(1.0f), scale);
			trans =  translate * rot * scaling;
		}
		else
		{
			trans = bone.mTransformation;
		}
		unsigned int parent = bone.mParentIndex;
		if (parent != -1)
		{
			bone.mCurrentTransformation = demoModel.mBones[parent].mCurrentTransformation * trans;
		}
		else
		{
			bone.mCurrentTransformation = trans;
		}
		
		mBonesTransformation.push_back(bone.mCurrentTransformation);
	}

	for (unsigned int i = 0; i < mBonesTransformation.size(); ++i)
	{
		modelDraw->SetUniformMat4f("boneMatrix[" + std::to_string(i) + "]", mBonesTransformation[i] * demoModel.mBones[i].mOffset);
	}
}

VertexArray AnimationScene::CreateBonesData()
{
	VertexArray va;
	VertexBuffer vb;

	std::vector<glm::vec3> vertices;

	for (unsigned int i = 1; i < demoModel.mBones.size(); ++i)
	{
		glm::mat4 trans = demoModel.mBones[i].mCurrentTransformation;
		vertices.push_back((glm::vec3)trans[3]);
		if (demoModel.mBones[i].mParentIndex != 0)
		{
			glm::mat4 parentTrans = demoModel.mBones[demoModel.mBones[i].mParentIndex].mCurrentTransformation;
			vertices.push_back((glm::vec3)parentTrans[3]);
		}
		else
		{
			vertices.push_back((glm::vec3)trans[3]);
		}
	}
	va.AddBuffer(vb);
	vb.AddData(&vertices[0], vertices.size() * sizeof(glm::vec3));

	va.Push(3, GL_FLOAT, sizeof(float));
	va.AddLayout();

	vb.Unbind();
	return va;
}
