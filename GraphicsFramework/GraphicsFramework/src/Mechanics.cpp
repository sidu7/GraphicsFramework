#include "Mechanics.h"
#include "../opengl/Renderer.h"
#include <glm/mat4x2.hpp>
#include <utility>
#include "ShapeManager.h"
#include <glm/ext/matrix_transform.inl>
#include "../opengl/Shader.h"
#include "Light.h"
#include "Engine.h"
#include "Camera.h"
#include "Time.h"

extern Engine* engine;

void Mechanics::Init()
{
	mLight = new Light();
	mLight->position = glm::vec3(10.0, 100.0, 40.0);

	engine->pCamera->mCameraPos = glm::vec3(20.0f, 0.0f, 100.0f);
	
	Drawing = new Shader("src/animshaders/Drawing.vert", "src/animshaders/Drawing.frag");

	NodeCount = 20;
	mSpringLength = 2.0f;
	mSpringConstant = 0.2f;
	mSpringFriction = 0.01f;
	mGravitationalForce = glm::vec3(0.0f,-9.8f,0.0f);
	mAirFriction = 0.02f;
	mRopeConnectionPosition = glm::vec3(0.0f, 6.0f, 0.0f);

	mSpringConstant = 20.0f * mGravitationalForce.y / mSpringLength;

	for(int i = 0; i < NodeCount; ++i)
	{
		Mass m{ 1.0f,glm::vec3(0.0f),glm::vec3(0.0f) };
		RopeNode node;
		node.mBody = m;
		node.mPosition = glm::vec3(i * mSpringLength, 4.0f, 0.0);
		node.mRotation = glm::vec3(0.0);
		node.mScale = glm::vec3(0.5, 0.5, 0.5);
		node.mModel = glm::mat4(1.0f);
		node.mRenderData = ShapeManager::Instance().mShapes[Shapes::CUBE];
		mRopeNodes.emplace_back(node);
	}

	for(int i = 0; i < NodeCount - 1; ++i)
	{
		Spring spring{ &mRopeNodes[i], &mRopeNodes[i + 1] };
		mSprings.emplace_back(spring);
	}
	
	/*RopeNode node;
	node.mPosition = glm::vec3(-5.0f - 20.0f, 4.0f, 0.0);
	node.mRotation = glm::vec3(0.0);
	node.mScale = glm::vec3(0.6, 0.6, 0.6);
	node.mModel = glm::mat4(1.0f);
	node.mRenderData = ShapeManager::Instance().mShapes[Shapes::SPHERE];	
	mRopeNodes.emplace_back(node);
	
	NodeCount = 6;
	for(int i = 0; i < NodeCount; ++i)
	{
		RopeNode node;
		node.mPosition = glm::vec3(i*5.0f - 20.0f, 4.0f, 0.0);
		node.mRotation = glm::vec3(0.0);
		node.mScale = glm::vec3(2.0, 0.4, 0.4);
		node.mModel = glm::mat4(1.0f);
		node.mRenderData = ShapeManager::Instance().mShapes[Shapes::CUBE];
		mRopeNodes.emplace_back(node);
	}

	node.mPosition = glm::vec3(NodeCount * 5.0f - 20.0f, 4.0f, 0.0);
	node.mRotation = glm::vec3(0.0);
	node.mScale = glm::vec3(0.6, 0.6, 0.6);
	node.mModel = glm::mat4(1.0f);
	node.mRenderData = ShapeManager::Instance().mShapes[Shapes::SPHERE];
	mRopeNodes.emplace_back(node);*/
}

void Mechanics::Draw()
{
	float deltaTime = Time::Instance().deltaTime/5.0f;

	float maxDT = 0.002f;

	int numIterations = (int)(deltaTime / maxDT) + 1;

	if(numIterations != 0)
	{
		deltaTime = deltaTime / numIterations;
	}

	for (int i = 0; i < numIterations; ++i)
	{
		Simulate(deltaTime);
	}
		
	Drawing->Bind();
	
	Drawing->SetUniform3f("lightPos", mLight->position.x, mLight->position.y, mLight->position.z);
	Drawing->SetUniform3f("light", 3.2f, 3.2f, 3.2f);
	Drawing->SetUniformMat4f("inverseview", glm::inverse(engine->pCamera->mView));
	Drawing->SetUniformMat4f("projection", engine->pCamera->mProjection);
	Drawing->SetUniformMat4f("view", engine->pCamera->mView);

	Drawing->SetUniform1i("lighting", 1);
	//Draw Rope
	for(int i = 0; i < mRopeNodes.size(); ++i)
	{
		RopeNode& node = mRopeNodes[i];
		glm::mat4 matrix = glm::mat4(1.0f);
		matrix = glm::translate(matrix,node.mPosition);
		matrix = glm::rotate(matrix, glm::radians(node.mRotation.x), glm::vec3(1.0, 0.0, 0.0));
		matrix = glm::rotate(matrix, glm::radians(node.mRotation.y), glm::vec3(0.0, 1.0, 0.0));
		matrix = glm::rotate(matrix, glm::radians(node.mRotation.z), glm::vec3(0.0, 0.0, 1.0));
		matrix = glm::scale(matrix, node.mScale);
		node.mModel = matrix;
		Drawing->SetUniformMat4f("model", node.mModel);
		Drawing->SetUniformMat4f("normaltr", glm::inverse(node.mModel));
		Drawing->SetUniform3f("diffuse", 1.0f, 0.0f, 0.0f);
		Drawing->SetUniform3f("specular", 0.2f, 0.2f, 0.2f);
		Drawing->SetUniform1f("shininess", 1.0f);		
		Renderer::Instance().Draw(*node.mRenderData.first, *node.mRenderData.second, *Drawing);
	}
	
	//Floor
	/*glm::mat4 matrix = glm::mat4(1.0f);
	matrix = glm::translate(matrix, glm::vec3(0.0f, 0.0f, 0.0f));
	matrix = glm::rotate(matrix, glm::radians(-90.0f), glm::vec3(1.0, 0.0, 0.0));
	matrix = glm::scale(matrix, glm::vec3(500.0f));
	Drawing->SetUniformMat4f("model", matrix);
	Drawing->SetUniformMat4f("normaltr", glm::inverse(matrix));
	Drawing->SetUniform3f("diffuse", 0.0f, 0.5f, 0.0f);
	Drawing->SetUniform3f("specular", 0.2f, 0.2f, 0.2f);
	Drawing->SetUniform1f("shininess", 1.0f);
	std::pair<VertexArray*, ElementArrayBuffer*> shape = ShapeManager::Instance().mShapes[Shapes::QUAD];
	Renderer::Instance().Draw(*shape.first, *shape.second, *Drawing);*/
}

void Mechanics::Simulate(float deltaTime)
{
	Start();
	Solve();
	Apply(deltaTime);
}

void Mechanics::Start()
{
	for(int i = 0; i < mRopeNodes.size(); ++i)
	{
		mRopeNodes[i].mBody.mForce = glm::vec3(0.0f);
	}
}

void Mechanics::Solve()
{
	//Apply Spring forces
	for(int i = 0; i < mSprings.size(); ++i)
	{
		Spring& spring = mSprings[i];
		glm::vec3 springVec = spring.mMass1->mPosition - spring.mMass2->mPosition;
		float r = glm::length(springVec);

		glm::vec3 force(0.0f);

		if(r > 0.0f)
		{
			force += -mSpringConstant * (springVec / r) * (r - mSpringLength);

			force += -(spring.mMass1->mBody.mVelocity - spring.mMass2->mBody.mVelocity) * mSpringFriction;

			spring.mMass1->mBody.mForce += force;
			spring.mMass2->mBody.mForce += -force;
		}
	}
	
	//Apply Gravitational forces
	for(int i = 0; i < mRopeNodes.size(); ++i)
	{
		Mass& mass = mRopeNodes[i].mBody;
		mass.mForce += mGravitationalForce * mass.mMass;
		//mass.mForce += -mAirFriction * mass.mMass;
	}
}

void Mechanics::Apply(float deltaTime)
{
	for(int i = 0; i < mRopeNodes.size(); ++i)
	{
		RopeNode& node = mRopeNodes[i];
		Mass& mass = node.mBody;

		mass.mVelocity += (mass.mForce/mass.mMass) * deltaTime;
		
		node.mPosition += mass.mVelocity * deltaTime;
	}

	mRopeNodes[0].mPosition = mRopeConnectionPosition;
}
