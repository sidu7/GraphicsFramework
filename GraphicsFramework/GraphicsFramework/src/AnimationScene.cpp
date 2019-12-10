#include "AnimationScene.h"
#include "Engine.h"
#include "Camera.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShapeManager.h"
#include <math.h>
#include "Time.h"
#include "ImguiManager.h"
#include <stack>
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/ext.hpp"
#include "glm/gtx/compatibility.hpp"
#include "glm/gtx/quaternion.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtc/type_ptr.hpp"

#include <fstream>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/filewritestream.h>
#include <rapidjson/ostreamwrapper.h>
#include <glm/gtx/quaternion.hpp>

extern Engine* engine;

std::string animation;
bool drawModel, drawBones;

void AnimationScene::Init()
{
	demoModel.LoadModel("res/ArcherWalking.fbx");
	demoModel.AddAnimation("res/ArcherDancing.fbx");
	demoModel.AddAnimation("res/ArcherShooting.fbx");
	demoModel.AddAnimation("res/PickUp.fbx");

	/*
	 *IK bones
	 *RightArm
	 *RightForeArm
	 *RightHand
	 */
	for(int i = demoModel.mBones.size() - 1; i >= 0 ; --i)
	{
		if(demoModel.mBones[i].mName == "RightArm" ||
			demoModel.mBones[i].mName == "RightForeArm" ||
			demoModel.mBones[i].mName == "RightHand" )
		{
			mIKBones.push_back(i);
		}
		demoModel.mBones[i].mIKTransformation = glm::mat4(1.0f);
		//std::cout << demoModel.mBones[i].mName << std::endl;
	}
	mEndEffector = glm::vec3(0.0f);
	mDoOnce = true;
	IKDuration = 1.0f;
	
	animation = demoModel.mAnimations[3];
	//demoModel.LoadModel("res/arissa.fbx");

	drawModel = true;
	drawBones = false;

	engine->pCamera->pitch = -8.0f;
	engine->pCamera->yaw = 4.8f;
	engine->pCamera->mCameraPos = glm::vec3(-126.0f, 26.0f, -14.0f);
	engine->pCamera->CalculateFront();

	mBonesTransformation.resize(demoModel.mBones.size());

	light = new Light();
	light->position = glm::vec3(10.0, 100.0, 40.0);

	modelDraw = new Shader("src/animshaders/ModelDraw.vert", "src/animshaders/ModelDraw.frag");
	Drawing = new Shader("src/animshaders/Drawing.vert", "src/animshaders/Drawing.frag");	

	std::ifstream file("res/JSON Data/ControlPoints.json");
	std::string contents((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>()); 
	file.close();
	rapidjson::Document root; 
	root.Parse(contents.c_str()); 
	if (!root.IsObject()) { std::cout << "Error reading ControlPoints.json" << std::endl; }

	rapidjson::Value::Array points = root["ControlPoints"].GetArray();
	controlPoints.clear();
	for (unsigned int i = 0; i < points.Size(); ++i)
	{
		rapidjson::Value::Array arr = points[i].GetArray();
		controlPoints.emplace_back(glm::vec4(arr[0].GetFloat(), arr[1].GetFloat(), arr[2].GetFloat(), 1.0f));
	}

	CurveMatrix[0] = glm::vec4(-1.0f, 3.0f, -3.0f, 1.0f);
	CurveMatrix[1] = glm::vec4(3.0f, -6.0f, 3.0f, 0.0f);
	CurveMatrix[2] = glm::vec4(-3.0f, 0.0f, 3.0f, 0.0f);
	CurveMatrix[3] = glm::vec4(1.0f, 4.0f, 1.0f, 0.0f);
	
	CurveMatrix /= 6.0f;

	CurveMatrix = glm::transpose(CurveMatrix);

	RecalculateMatrices();

	mSpeed = 18.0f;
	mTolerance = 0.0001f;
	CreateAxisLengthTable();	

	CreateControlPointsVAO();

	mPathMatrix = glm::mat4(1.0f);

	showControlWindow = false;
	mGoalPosition = glm::vec3(-5.0f, 14.0f, 10.0f);
	maxIterations = 10;
	sqrDistanceError = 0.2f;
}

void AnimationScene::Draw()
{
	modelDraw->Bind();

	if (!isPaused)
	{
		PathRunTime += Time::Instance().deltaTime;
	}

	glEnable(GL_DEPTH_TEST);

	ImGuiWindow();

	modelDraw->SetUniform3f("lightPos", light->position.x, light->position.y, light->position.z);
	modelDraw->SetUniform3f("light", 3.2f, 3.2f, 3.2f);
	modelDraw->SetUniformMat4f("inverseview", glm::inverse(engine->pCamera->mView));
	modelDraw->SetUniformMat4f("projection", engine->pCamera->mProjection);
	modelDraw->SetUniformMat4f("view", engine->pCamera->mView);
	
	float speed = mSpeed;
	if(mPathWalk)
	{
		float distance;
		if (PathRunTime < t1)
		{
			speed = mSpeed * PathRunTime / t1;
			distance = 0.5f * mSpeed / t1 * PathRunTime * PathRunTime;
		}
		else if (PathRunTime > t1&& PathRunTime <= t2)
		{
			speed = mSpeed;
			distance = mSpeed * t1 * 0.5f + mSpeed * (PathRunTime - t1);
		}
		else if (PathRunTime > t2&& PathRunTime <= totalRunTime)
		{
			speed = mSpeed * (totalRunTime - PathRunTime) / (totalRunTime - t2);
			distance = mSpeed * t1 * 0.5f + mSpeed * (t2 - t1) + (mSpeed - (mSpeed * (PathRunTime - t2) / (totalRunTime - t2)) * 0.5f) *
				(PathRunTime - t2);
		}
		else
		{
			PathRunTime = 0.0f;
			distance = 0.0f;
			speed = 0.0f;
		}

		//float distance = mSpeed * PathRunTime;
		std::pair<float, int> searchedValue = SearchInTable(distance);
		glm::vec3 position = GetPointOnCurve(searchedValue.first, controlPointsMatrices[searchedValue.second]);
		mPathMatrix = glm::translate(glm::mat4(1.0f), position);
		glm::vec3 viewDirection = GetDerivativeOfPointOnCurve(searchedValue.first, controlPointsMatrices[searchedValue.second]);
		viewDirection = glm::normalize(viewDirection);
		glm::vec3 wingDirection = glm::normalize(glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), viewDirection));
		glm::vec3 upDirection = glm::normalize(glm::cross(viewDirection, wingDirection));

		glm::mat4 rotation;
		rotation[0] = glm::vec4(wingDirection, 0.0f);
		rotation[1] = glm::vec4(upDirection, 0.0f);
		rotation[2] = glm::vec4(viewDirection, 0.0f);
		rotation[3] = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

		mPathMatrix *= rotation;
	}
	
	if (!isPaused)
	{
		float speedFactor = speed / mSpeed;
		AnimationRunTime += Time::Instance().deltaTime * speedFactor;
	}

	if (mDoOnce)
	{
		AnimatorUpdate(animation);
		mDoOnce = false;
	}
	
	glm::mat4 matrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));
	matrix = glm::scale(matrix, glm::vec3(0.1f));

	if(mEndEffector == glm::vec3(0.0f))
	{
		mEndEffector = demoModel.mBones[mIKBones[0]].mCurrentGlobalTransformation[3];
	}
	
	
	IKUpdate(matrix, AnimationRunTime/IKDuration);

	for (unsigned int i = 0; i < demoModel.mBones.size(); ++i)
	{
		modelDraw->SetUniformMat4f("boneMatrix[" + std::to_string(i) + "]", demoModel.mBones[i].mCurrentGlobalTransformation * demoModel.mBones[i].mOffset);
	}
	
	if (drawModel)
	{
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
		modelDraw->SetUniformMat4f("pathTr", mPathMatrix);
		modelDraw->SetUniformMat4f("model", matrix);
		modelDraw->SetUniformMat4f("normaltr", glm::inverse(mPathMatrix * matrix));
		modelDraw->SetUniform3f("diffuse", 1.0f, 0.0f, 0.0f);
		modelDraw->SetUniform3f("specular", 1.0f, 1.0f, 1.0f);
		modelDraw->SetUniform1f("shininess", 1.0f);

		demoModel.Draw(modelDraw);
		glDisable(GL_CULL_FACE);
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
			Drawing->SetUniformMat4f("model", mPathMatrix * matrix * demoModel.mBones[i].mCurrentGlobalTransformation);
			Drawing->SetUniform3f("diffuse", 1.0f, 0.0f, 0.0f);
			std::pair<VertexArray*, ElementArrayBuffer*> shape = ShapeManager::Instance().mShapes[Shapes::CUBE];
			Renderer::Instance().Draw(*shape.first, *shape.second, *Drawing);
		}
		VertexArray VAO = CreateBonesData();
		VAO.Bind();
		//glLineWidth(1.0f);
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
	
	mCurveVAO.Bind();
	//glLineWidth(2.0f);
	glDrawArrays(GL_LINES, 1, mCurvePointsSize - 1);
	mCurveVAO.Unbind();
	
	matrix = glm::mat4(1.0f);
	matrix = glm::translate(matrix, glm::vec3(0.0f, 0.0f, 0.0f));
	matrix = glm::rotate(matrix, glm::radians(-90.0f), glm::vec3(1.0, 0.0, 0.0));
	matrix = glm::scale(matrix, glm::vec3(500.0f));
	Drawing->SetUniformMat4f("model", matrix);
	Drawing->SetUniformMat4f("normaltr", glm::inverse(matrix));
	Drawing->SetUniform3f("diffuse", 0.0f, 0.5f, 0.0f);
	Drawing->SetUniform3f("specular", 0.2f, 0.2f, 0.2f);
	Drawing->SetUniform1f("shininess", 1.0f);
	std::pair<VertexArray*, ElementArrayBuffer*> shape = ShapeManager::Instance().mShapes[Shapes::QUAD];
	Renderer::Instance().Draw(*shape.first, *shape.second, *Drawing);

	matrix = glm::mat4(1.0f);
	matrix = glm::translate(matrix, mGoalPosition);
	matrix = glm::scale(matrix, glm::vec3(1.0f));
	Drawing->SetUniformMat4f("model", matrix);
	Drawing->SetUniformMat4f("normaltr", glm::inverse(matrix));
	Drawing->SetUniform3f("diffuse", 0.5f, 0.0f, 0.0f);
	Drawing->SetUniform3f("specular", 0.2f, 0.2f, 0.2f);
	Drawing->SetUniform1f("shininess", 1.0f);
	shape = ShapeManager::Instance().mShapes[Shapes::SPHERE];
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
			timeFrame = anim.mDuration - 2; // Static IK pose
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
		bone.mCurrentLocalTransformation = trans;
		if (parent != -1)
		{
			bone.mCurrentGlobalTransformation = demoModel.mBones[parent].mCurrentGlobalTransformation * trans;
		}
		else
		{
			bone.mCurrentGlobalTransformation = trans;
		}
		
		//mBonesTransformation.push_back(bone.mCurrentGlobalTransformation);
	}
}

VertexArray AnimationScene::CreateBonesData()
{
	VertexArray va;
	VertexBuffer vb;

	std::vector<glm::vec3> vertices;

	for (unsigned int i = 1; i < demoModel.mBones.size(); ++i)
	{
		glm::mat4 trans = demoModel.mBones[i].mCurrentGlobalTransformation;
		vertices.push_back((glm::vec3)trans[3]);
		if (demoModel.mBones[i].mParentIndex != 0)
		{
			glm::mat4 parentTrans = demoModel.mBones[demoModel.mBones[i].mParentIndex].mCurrentGlobalTransformation;
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

glm::vec4 AnimationScene::GetPointOnCurve(float t, glm::mat4& controlPointMatrix)
{
	return glm::vec4(t*t*t, t*t, t, 1.0f) * CurveMatrix * controlPointMatrix;
}

glm::vec4 AnimationScene::GetDerivativeOfPointOnCurve(float t, glm::mat4& controlPointMatrix)
{
	return glm::vec4(3 * t * t, 2 * t, 1.0f, 0.0f) * CurveMatrix * controlPointMatrix;
}

void AnimationScene::Deserialize()
{
	std::ofstream file("res/JSON Data/ControlPoints.json");
	rapidjson::StringBuffer s;
	rapidjson::Writer<rapidjson::StringBuffer> writer(s);
	writer.StartObject();
	writer.Key("ControlPoints");
	writer.StartArray();
	for (unsigned int i = 0; i < controlPoints.size(); ++i)
	{
		writer.StartArray();
		writer.Double(controlPoints[i].x);
		writer.Double(controlPoints[i].y);
		writer.Double(controlPoints[i].z);
		writer.EndArray();
	}
	writer.EndArray();
	writer.EndObject();
	file.write(s.GetString(), s.GetSize());
	std::cout << "Successfully written to File" << std::endl;
}

void AnimationScene::CreateControlPointsVAO()
{
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
	mCurvePointsSize = curvePoints.size();
	mCurveVAO = CreateVec4VAO(curvePoints);
}

void AnimationScene::ImGuiWindow()
{
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
	ImGui::Checkbox("Walk on Path", &mPathWalk);
	if (ImGui::Button(showControlWindow ? "Hide Control Points Window" : "Show Control Points Window"))
	{
		showControlWindow = !showControlWindow;
	}
	if (ImGui::InputFloat("ArcTable Tolerance", &mTolerance, 0.0005f,0.005f,4))
	{
		mTolerance = mTolerance < 0.0f ? 0.0001f : mTolerance;
		CreateAxisLengthTable();
	}
	ImGui::InputFloat("IK Duration", &IKDuration, 0.5f);
	ImGui::Text("GoalPosition");
	if(ImGui::InputFloat("X", &mGoalPosition.x, 0.2f) ||
	ImGui::InputFloat("Y", &mGoalPosition.y, 0.2f) ||
	ImGui::InputFloat("Z", &mGoalPosition.z, 0.2f))
	{
		AnimationRunTime = 0.0f;
		mEndEffector = demoModel.mBones[mIKBones[0]].mCurrentGlobalTransformation[3];
	}
	
	if (showControlWindow)
	{
		ImGui::Begin("Control Points");
		if (ImGui::Button("New Control Point"))
		{
			controlPoints.emplace_back(controlPoints[controlPoints.size() - 1]);
			RecalculateMatrices();
			CreateAxisLengthTable();
			CreateControlPointsVAO();
		}
		ImGui::SameLine();
		if (ImGui::Button("Save Control Points"))
		{
			Deserialize();
		}
		for (unsigned int i = 0; i < controlPoints.size(); ++i)
		{
			if (ImGui::SliderFloat3(std::string("Point" + std::to_string(i + 1)).c_str(), (float*)&controlPoints[i], -150.0f, 150.0f, "%.1f"))
			{
				controlPoints[i].y = 0.0f;
				RecalculateMatrices();
				CreateAxisLengthTable();
				CreateControlPointsVAO();
			}
		}
		ImGui::End();
	}
	ImGui::End();
}

void AnimationScene::IKUpdate(glm::mat4 model, float time)
{
	glm::vec3 Goal = glm::inverse(model) * glm::vec4(mGoalPosition,1.0);
	glm::vec3 currentEndEffector = (model * demoModel.mBones[mIKBones[0]].mCurrentGlobalTransformation)[3];
	if (time > 1.0f) time = 1.0f;
	glm::vec3 Gprime = (1 - time) * mEndEffector + (time)*Goal;
	//Gprime = Goal;
	float sqrDistance = glm::length(currentEndEffector - mGoalPosition);

	int iterationCount = 0;
	while (sqrDistance > sqrDistanceError && iterationCount <= maxIterations)
	{
		//for (int i = 0; i < mIKBones.size() - 2; ++i)
		{
			//for (int j = 1; j < i + 3 && j < mIKBones.size(); ++j)
			for (int j = 1; j < mIKBones.size(); ++j)
			{
				glm::vec3 originOfRotation = demoModel.mBones[mIKBones[j]].mCurrentGlobalTransformation[3];
				currentEndEffector = demoModel.mBones[mIKBones[0]].mCurrentGlobalTransformation[3];				
				glm::vec3 B = glm::normalize(currentEndEffector - originOfRotation);
				glm::vec3 C = glm::normalize(Gprime - originOfRotation);
				float dot = glm::dot(B, C);
				if(dot >= 0.9999f)
				{
					continue;
				}
				float alpha = glm::acos(dot);
				if (j == 1)
				{
					glm::vec3 shoulder = demoModel.mBones[mIKBones[j + 1]].mCurrentGlobalTransformation[3];
					glm::vec3 biceps = glm::normalize(shoulder - originOfRotation);
					float dot1 = glm::dot(biceps, C);
					if (dot1 >= 0.9999f)
					{
						continue;
					}
					float alpha1 = glm::acos(dot1);
					if (alpha1 >= 2.8)
					{
						continue;
					}
				}				
				glm::vec3 axis = glm::cross(B, C);
				axis = glm::inverse(demoModel.mBones[mIKBones[j]].mCurrentGlobalTransformation) * glm::vec4(axis, 0.0f);
				float alen = glm::length(axis);
				glm::quat q = glm::quat(cos(alpha / 2.0f), sin(alpha / 2.0f) * axis.x / alen,
					sin(alpha / 2.0f) * axis.y / alen, sin(alpha / 2.0f) * axis.z / alen);
				glm::normalize(q);
				demoModel.mBones[mIKBones[j]].mIKTransformation = demoModel.mBones[mIKBones[j]].mIKTransformation *
					glm::toMat4(q);
				//demoModel.mBones[mIKBones[j]].mIKTransformation = glm::toMat4(q);
				
				for (int k = j; k >= 0; --k)
				{
					Bone& bone = demoModel.mBones[mIKBones[k]];
					bone.mCurrentGlobalTransformation = demoModel.mBones[bone.mParentIndex].mCurrentGlobalTransformation *
							bone.mCurrentLocalTransformation * bone.mIKTransformation;
				}

				currentEndEffector = (model * demoModel.mBones[mIKBones[0]].mCurrentGlobalTransformation)[3];

				sqrDistance = glm::length(currentEndEffector - mGoalPosition);

				if (sqrDistance <= sqrDistanceError)
				{
					goto Update_children;
				}
			}
		}

		currentEndEffector = (model * demoModel.mBones[mIKBones[0]].mCurrentGlobalTransformation)[3];
		sqrDistance = glm::length(currentEndEffector - mGoalPosition);
		++iterationCount;
	}

	Update_children:
	//Update end effectors children
	for (unsigned int i = 0; i < demoModel.mBones.size(); ++i)
	{
		if(std::find(mIKBones.begin(),mIKBones.end(),i) != mIKBones.end())
		{
			continue;
		}
		Bone& bone = demoModel.mBones[i];
		if (bone.mParentIndex != -1)
		{
			bone.mCurrentGlobalTransformation = demoModel.mBones[bone.mParentIndex].mCurrentGlobalTransformation *
				bone.mCurrentLocalTransformation;
		}
	}
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
	mArcLengthTable.clear();
	mArcLengthTable.emplace_back(MAKE_TUPLE(0.0f, 0.0f, 0));
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

			if (A + B - C > mTolerance)
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

	totalRunTime = mArcLengthTable[mArcLengthTable.size() - 1].first / mSpeed;
	
	//V((t1 + (t3 - t2)) / 2)
	float t3 = totalRunTime;
	t1 = totalRunTime * 0.2f;
	t2 = totalRunTime * 0.9f;

	totalRunTime += ((t3 - t2) + t1);
}

inline std::pair<float,int> AnimationScene::SearchInTable(float distance)
{
	for (unsigned int i = 0; i < mArcLengthTable.size(); ++i)
	{
		float tableDistance = mArcLengthTable[i].first;
		if (tableDistance > distance)
		{
			float factor = (distance - mArcLengthTable[i - 1].first) / (tableDistance - mArcLengthTable[i - 1].first);
			float s; int index;			
			float T2 = mArcLengthTable[i].second.first;
			float T1 = 0.0f;
			if (mArcLengthTable[i - 1].second.second == mArcLengthTable[i].second.second)
			{
				T1 = mArcLengthTable[i - 1].second.first;
			}
			s = glm::lerp(T1, T2, factor);
			index = i;
			return std::make_pair(s, mArcLengthTable[index].second.second);
		}
	}
	PathRunTime = 0.0f;
	return std::make_pair(0, 0);
}

inline glm::mat4 AnimationScene::RotationFromDirection(const glm::vec3& direction)
{
	float angle = std::atan2(direction.y, direction.x);
	glm::mat4 glmrotXY = glm::rotate(angle, glm::vec3(0.0f, 0.0f, 1.0f));
	// Find the angle with the xy with plane (0, 0, 1); the - there is because we want to 
	// 'compensate' for that angle (a 'counter-angle')
	float angleZ = -std::asin(direction.z);
	// Make the matrix for that, assuming that Y is your 'side' vector; makes the model 'pitch'
	glm::mat4 glmrotZ = glm::rotate(angleZ, glm::vec3(0.0f, 1.0f, 0.0f));
	return glmrotXY * glmrotZ;
}