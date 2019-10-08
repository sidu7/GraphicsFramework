#include "AnimationScene.h"
#include "Engine.h"
#include "Camera.h"
#include "glm/gtc/matrix_transform.hpp"
#include "ShapeManager.h"
#include <math.h>
#include "Time.h"
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/compatibility.hpp"

extern Engine* engine;

void AnimationScene::Init()
{
	demoModel.LoadModel("res/gh_sample_animation.fbx");

	mBonesTransformation.resize(demoModel.mBones.size());

	light = new Light();
	light->position = glm::vec3(10.0, 100.0, 40.0);

	modelDraw = new Shader("src/animshaders/ModelDraw.vert", "src/animshaders/ModelDraw.frag");
	Drawing = new Shader("src/animshaders/Drawing.vert", "src/animshaders/Drawing.frag");
}

void AnimationScene::Draw()
{
	modelDraw->Bind();

	RunTime += Time::Instance().deltaTime;

	AnimatorUpdate("Armature|walk");

	modelDraw->SetUniform3f("lightPos", light->position.x, light->position.y, light->position.z);
	modelDraw->SetUniform3f("light", 3.2f, 3.2f, 3.2f);
	modelDraw->SetUniformMat4f("inverseview", glm::inverse(engine->pCamera->mView));
	modelDraw->SetUniformMat4f("projection", engine->pCamera->mProjection);
	modelDraw->SetUniformMat4f("view", engine->pCamera->mView);

	glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 7.5f, 0.0f));
	model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0, 0.0));
	modelDraw->SetUniformMat4f("model", model);
	modelDraw->SetUniformMat4f("normaltr", glm::inverse(model));
	modelDraw->SetUniform3f("diffuse", 1.0f, 0.0f, 0.0f);
	modelDraw->SetUniform3f("specular", 1.0f, 1.0f, 1.0f);
	modelDraw->SetUniform1f("shininess", 10.0f);

	demoModel.Draw(modelDraw);

	Drawing->Bind();
	Drawing->SetUniform3f("lightPos", light->position.x, light->position.y, light->position.z);
	Drawing->SetUniform3f("light", 3.2f, 3.2f, 3.2f);
	Drawing->SetUniformMat4f("inverseview", glm::inverse(engine->pCamera->mView));
	Drawing->SetUniformMat4f("projection", engine->pCamera->mProjection);
	Drawing->SetUniformMat4f("view", engine->pCamera->mView);

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

			glm::vec3 position = posIndex == 0 ? anim.mKeyPositions[0].second : glm::lerp(anim.mKeyPositions[rotIndex - 1].second, anim.mKeyPositions[rotIndex].second, (float)timeFrame);
			Quaternion rotation = rotIndex == 0 ? anim.mKeyRotations[0].second : Quaternion::interpolate(anim.mKeyRotations[rotIndex - 1].second, anim.mKeyRotations[rotIndex].second, timeFrame);
			glm::vec3 scale = sclIndex == 0 ? anim.mKeyScalings[0].second : glm::lerp(anim.mKeyScalings[rotIndex - 1].second, anim.mKeyScalings[rotIndex].second, (float)timeFrame);
			glm::mat4 translate = glm::translate(glm::mat4(1.0f), position);
			glm::mat4 scaling = glm::scale(glm::mat4(1.0f), scale);
			trans =  translate * rotation.matrix() * scaling;
		}
		else
		{
			trans = bone.mTransformation;
		}
		
		unsigned int parent = bone.mParentIndex;
		while (parent != -1)
		{
			if (demoModel.mBones[parent].isAnimated)
			{
				trans = demoModel.mBones[parent].mCurrentTransformation * trans;
			}
			else
			{
				trans = demoModel.mBones[parent].mTransformation * trans;
			}
			parent = demoModel.mBones[parent].mParentIndex;
		}
		bone.mCurrentTransformation = trans;
		
		mBonesTransformation.push_back(bone.mCurrentTransformation);
	}

	for (unsigned int i = 0; i < mBonesTransformation.size(); ++i)
	{
		modelDraw->SetUniformMat4f("boneMatrix[" + std::to_string(i) + "]", mBonesTransformation[i] * demoModel.mBones[i].mOffset);
	}
}
