#include "AnimationScene.h"
#include "Engine.h"
#include "Camera.h"
#include "glm/gtc/matrix_transform.hpp"
#include "ShapeManager.h"
#include <math.h>
#include "Time.h"
#include "ImguiManager.h"
#include <stack>
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/compatibility.hpp"
#include "glm/gtc/type_ptr.hpp"

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
	engine->pCamera->mCameraPos = glm::vec3(0.0f, 50.0f, 150.0f);
	engine->pCamera->CalculateFront();

	mBonesTransformation.resize(demoModel.mBones.size());

	light = new Light();
	light->position = glm::vec3(10.0, 100.0, 40.0);

	modelDraw = new Shader("src/animshaders/ModelDraw.vert", "src/animshaders/ModelDraw.frag");
	Drawing = new Shader("src/animshaders/Drawing.vert", "src/animshaders/Drawing.frag");	

	controlPoints.emplace_back(glm::vec4(-25.0f, 0.0f, 0.0f, 1.0f));
	controlPoints.emplace_back(glm::vec4(-20.0f, 0.0f, 20.0f, 1.0f));
	controlPoints.emplace_back(glm::vec4(20.0f, 0.0f, 20.0f, 1.0f));
	controlPoints.emplace_back(glm::vec4(25.0f, 0.0f, 0.0f, 1.0f));
	controlPoints.emplace_back(glm::vec4(45.0f, 0.0f, 15.0f, 1.0f));
	controlPoints.emplace_back(glm::vec4(25.0f, 0.0f, 45.0f, 1.0f));

	CurveMatrix[0] = glm::vec4(-1.0f, 3.0f, -3.0f, 1.0f);
	CurveMatrix[1] = glm::vec4(3.0f, -6.0f, 3.0f, 0.0f);
	CurveMatrix[2] = glm::vec4(-3.0f, 0.0f, 3.0f, 0.0f);
	CurveMatrix[3] = glm::vec4(1.0f, 4.0f, 1.0f, 0.0f);
	
	CurveMatrix /= 6.0f;

	CurveMatrix = glm::transpose(CurveMatrix);

	RecalculateMatrices();

	mArcLengthTable.emplace_back(MAKE_TUPLE(0.0f, 0.0f, 0));
	CreateAxisLengthTable();	

	mPosition = glm::vec3(0.0f);
	mPathMatrix = glm::mat4(1.0f);

	mSpeed = 9.0f;
}

void AnimationScene::Draw()
{
	modelDraw->Bind();

	if (!isPaused)
	{
		AnimationRunTime += Time::Instance().deltaTime/2.0f;
		PathRunTime += Time::Instance().deltaTime;
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
				AnimationRunTime = 0.01f;
			}
		}
		ImGui::EndCombo();
	}
	ImGui::Checkbox("Draw Skin", &drawModel);
	ImGui::Checkbox("Draw Bones", &drawBones);
	if (ImGui::Button("Step --"))
	{
		isPaused = true;
		AnimationRunTime -= 2.0f * Time::Instance().deltaTime;
	}
	ImGui::SameLine();
	if (ImGui::Button("Step -"))
	{
		isPaused = true;
		AnimationRunTime -= Time::Instance().deltaTime;
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
		AnimationRunTime += Time::Instance().deltaTime;
	}
	ImGui::SameLine();
	if (ImGui::Button("Step ++"))
	{
		isPaused = true;
		AnimationRunTime += 2.0f * Time::Instance().deltaTime;
	}
	static float point[2];
	if (ImGui::SliderFloat2("Control Point", point, -150.0f, 150.0f, "%.1f"))
	{
		controlPoints[0].x = point[0];
		controlPoints[0].z = point[1];

		RecalculateMatrices();
	}
	ImGui::InputFloat("Velocity", &mSpeed, 0.25f);

	ImGui::End();

	AnimatorUpdate(animation);

	modelDraw->SetUniform3f("lightPos", light->position.x, light->position.y, light->position.z);
	modelDraw->SetUniform3f("light", 3.2f, 3.2f, 3.2f);
	modelDraw->SetUniformMat4f("inverseview", glm::inverse(engine->pCamera->mView));
	modelDraw->SetUniformMat4f("projection", engine->pCamera->mProjection);
	modelDraw->SetUniformMat4f("view", engine->pCamera->mView);

	glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));
	model = glm::scale(model, glm::vec3(0.1f));
	mPosition += glm::vec3(0.0f,0.0f,mSpeed) * Time::Instance().deltaTime;
	float distance = mSpeed * PathRunTime;
	glm::vec3 position = SearchInTable(distance);
	mPathMatrix = glm::translate(glm::mat4(1.0f), position);

	if (drawModel)
	{
		modelDraw->SetUniformMat4f("pathTr", mPathMatrix);
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
		VertexArray VAO = CreateBonesData();
		VAO.Bind();
		glDrawArrays(GL_LINES, 0, demoModel.mBones.size() * 2 - 2);
		VAO.Unbind();		
		Drawing->SetUniform1i("lighting", 1);
	}

	glEnable(GL_DEPTH_TEST);

	for (unsigned int i = 0; i < controlPoints.size(); ++i)
	{
		Drawing->Bind();
		glm::mat4 controlModel = glm::translate(glm::mat4(1.0f), glm::vec3(controlPoints[i]));
		controlModel = glm::translate(controlModel, glm::vec3(0.0f,0.125f,0.0f));
		controlModel = glm::scale(controlModel, glm::vec3(0.25f));
		Drawing->SetUniformMat4f("model", controlModel);
		Drawing->SetUniform3f("diffuse", 0.0f, 0.0f, 1.0f);
		std::pair<VertexArray*, ElementArrayBuffer*> shape = ShapeManager::Instance().mShapes[Shapes::CUBE];
		Renderer::Instance().Draw(*shape.first, *shape.second, *Drawing);
	}

	Drawing->Bind();
	Drawing->SetUniformMat4f("model", glm::mat4(1.0f));
	std::vector<glm::vec4> curvePoints;
	for (unsigned int j = 0; j < controlPointsMatrices.size(); ++j)
	{
		for (float i = 0.0f; i <= 1.0f; i += 0.01f)
		{
			glm::vec4 pointOnCurve = GetPointOnCurve(i, controlPointsMatrices[j]);
			curvePoints.emplace_back(pointOnCurve);
			curvePoints.emplace_back(pointOnCurve);
		}
	}
	VertexArray va = CreateVec4VAO(curvePoints);
	va.Bind();
	glDrawArrays(GL_LINES, 1, curvePoints.size() - 1);
	va.Unbind();
	
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
			double TimeinTicks = AnimationRunTime * anim.mTicksPerSec;
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
	va.Unbind();
	return va;
}

inline glm::vec4 AnimationScene::GetPointOnCurve(float t, glm::mat4& controlPointMatrix)
{		
	return glm::vec4(t*t*t, t*t, t, 1.0f) * CurveMatrix * controlPointMatrix;
}

VertexArray AnimationScene::CreateVec4VAO(std::vector<glm::vec4>& pointList)
{
	VertexArray va;
	VertexBuffer vb;

	va.AddBuffer(vb);
	vb.AddData(&pointList[0], pointList.size() * sizeof(glm::vec4));

	va.Push(4, GL_FLOAT, sizeof(float));
	va.AddLayout();

	vb.Unbind();
	va.Unbind();
	return va;
}

void AnimationScene::RecalculateMatrices()
{
	controlPointsMatrices.clear();

	for (unsigned int i = 0; i <= controlPoints.size() - 4; ++i)
	{
		glm::mat4 matrix;
		matrix[0] = controlPoints[i];
		matrix[1] = controlPoints[i + 1];
		matrix[2] = controlPoints[i + 2];
		matrix[3] = controlPoints[i + 3];

		controlPointsMatrices.emplace_back(glm::transpose(matrix));
	}
}

void AnimationScene::CreateAxisLengthTable()
{
	for (unsigned int i = 0; i < controlPointsMatrices.size(); ++i)
	{
		std::stack<TABLE_ENTRY> adaptiveAlgoStack;
		adaptiveAlgoStack.push(MAKE_TUPLE(0.0f,1.0f, i));

		while (adaptiveAlgoStack.size() > 0)
		{
			TABLE_ENTRY topValue = adaptiveAlgoStack.top();
			adaptiveAlgoStack.pop();

			int index = topValue.second.second;
			float sa = topValue.first;
			float sb = topValue.second.first;
			float sm = (sa + sb) / 2;

			glm::vec4 Psm = GetPointOnCurve(sm, controlPointsMatrices[index]);
			glm::vec4 Psa = GetPointOnCurve(sa, controlPointsMatrices[index]);
			glm::vec4 Psb = GetPointOnCurve(sb, controlPointsMatrices[index]);

			float A = glm::length(Psm - Psa);
			float B = glm::length(Psb - Psm);
			float C = glm::length(Psb - Psa);

			if (A + B - C > 0.0001f)
			{
				adaptiveAlgoStack.push(MAKE_TUPLE(sm, sb, index));
				adaptiveAlgoStack.push(MAKE_TUPLE(sa, sm, index));
			}
			else
			{
				unsigned int prevIndex = mArcLengthTable.size();
				float distance = mArcLengthTable[prevIndex - 1].first;
				mArcLengthTable.emplace_back(MAKE_TUPLE(distance + A, sm, index));
				mArcLengthTable.emplace_back(MAKE_TUPLE(distance + A + B,sb,index));
			}
		}
	}
}

inline glm::vec3 AnimationScene::SearchInTable(float distance)
{
	for (unsigned int i = 0; i < mArcLengthTable.size(); ++i)
	{
		float tableDistance = mArcLengthTable[i].first;
		if (tableDistance > distance)
		{
			float factor = (distance - mArcLengthTable[i - 1].first) / (tableDistance - mArcLengthTable[i - 1].first);
			float T2 = mArcLengthTable[i].second.first;
			float T1 = mArcLengthTable[i - 1].second.first;
			float s = glm::lerp(T1, T2, factor);
			return GetPointOnCurve(s, controlPointsMatrices[mArcLengthTable[i].second.second]);
		}
	}
}
