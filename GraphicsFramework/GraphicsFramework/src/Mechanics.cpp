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
#include "Inputs.h"
#include <Imgui/imgui.h>

extern Engine* engine;

void Mechanics::Init()
{
	mLight = new Light();
	mLight->position = glm::vec3(10.0, 100.0, 40.0);

	engine->pCamera->mCameraPos = glm::vec3(20.0f, 23.0f, 60.0f);	
	engine->pCamera->pitch = -20.0f;
	engine->pCamera->CalculateFront();
	
	Drawing = new Shader("src/animshaders/Drawing.vert", "src/animshaders/Drawing.frag");

	NodeCount = 80;
	Time = 0.0f;
	NodeMass = 0.05f;
	mSpringLength = 0.05f;
	mSpringConstant = 30.0f;
	mSpringFriction = 0.2f;
	mGravitationalForce = glm::vec3(0.0f,-9.8f,0.0f);
	mAirFriction = 0.02f;
	mRopeConnectionPosition1 = glm::vec3(0.0f, 6.0f, 0.0f);
	mRopeConnectionPosition2 = glm::vec3(40.0f, 6.0f, 0.0f);
	mRopeConnectionVelocity = 50.0f;

	//mSpringFriction = NodeCount * NodeMass * -mGravitationalForce.y / (mSpringLength/10.0f);

	for(int i = 0; i < NodeCount; ++i)
	{
		Mass m{ NodeMass,glm::vec3(0.0f),glm::vec3(0.0f) };
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
	ImGui::Begin("Mechanics");
	ImGui::InputFloat("Rope Friction Constant", &mSpringFriction);
	ImGui::InputFloat("Rope Connection Velocity", &mRopeConnectionVelocity,0.5);
	ImGui::InputFloat("Air Friction", &mAirFriction);
	ImGui::End();

	
	float deltaTime = Time::Instance().deltaTime;

	Time += deltaTime;
		
	float maxDT = 0.016f;

	int numIterations = (int)(deltaTime / maxDT) + 1;

	if(numIterations != 0)
	{
		deltaTime = deltaTime / numIterations;
	}

	for (int i = 0; i < numIterations; ++i)
	{
		Simulate(deltaTime);
	}

	deltaTime = Time::Instance().deltaTime;
	
	if (Inputs::Instance().IsPressed(SDL_SCANCODE_LSHIFT))
	{
		if (Inputs::Instance().IsPressed(SDL_SCANCODE_Y))
		{
			mRopeConnectionPosition1 += glm::vec3(0.0f,mRopeConnectionVelocity * deltaTime,0.0f);
		}
		if(Inputs::Instance().IsPressed(SDL_SCANCODE_H))
		{
			mRopeConnectionPosition1 -= glm::vec3(0.0f, mRopeConnectionVelocity * deltaTime, 0.0f);
		}
		if (Inputs::Instance().IsPressed(SDL_SCANCODE_UP))
		{
			mRopeConnectionPosition2 += glm::vec3(0.0f, mRopeConnectionVelocity * deltaTime, 0.0f);
		}
		if (Inputs::Instance().IsPressed(SDL_SCANCODE_DOWN))
		{
			mRopeConnectionPosition2 -= glm::vec3(0.0f, mRopeConnectionVelocity * deltaTime, 0.0f);
		}
	}
	else
	{
		if (Inputs::Instance().IsPressed(SDL_SCANCODE_Y))
		{
			mRopeConnectionPosition1 -= glm::vec3(0.0f, 0.0f,mRopeConnectionVelocity * deltaTime);
		}
		if (Inputs::Instance().IsPressed(SDL_SCANCODE_H))
		{
			mRopeConnectionPosition1 += glm::vec3(0.0f, 0.0f, mRopeConnectionVelocity * deltaTime);
		}
		if (Inputs::Instance().IsPressed(SDL_SCANCODE_G))
		{
			mRopeConnectionPosition1 -= glm::vec3( mRopeConnectionVelocity * deltaTime, 0.0f, 0.0f);
		}
		if (Inputs::Instance().IsPressed(SDL_SCANCODE_J))
		{
			mRopeConnectionPosition1 += glm::vec3(mRopeConnectionVelocity * deltaTime, 0.0f, 0.0f);
		}
		if (Inputs::Instance().IsPressed(SDL_SCANCODE_UP))
		{
			mRopeConnectionPosition2 -= glm::vec3(0.0f, 0.0f, mRopeConnectionVelocity * deltaTime);
		}
		if (Inputs::Instance().IsPressed(SDL_SCANCODE_DOWN))
		{
			mRopeConnectionPosition2 += glm::vec3(0.0f, 0.0f, mRopeConnectionVelocity * deltaTime);
		}
		if (Inputs::Instance().IsPressed(SDL_SCANCODE_RIGHT))
		{
			mRopeConnectionPosition2 += glm::vec3(mRopeConnectionVelocity * deltaTime, 0.0f, 0.0f);
		}
		if (Inputs::Instance().IsPressed(SDL_SCANCODE_LEFT))
		{
			mRopeConnectionPosition2 -= glm::vec3(mRopeConnectionVelocity * deltaTime, 0.0f, 0.0f);
		}
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

void Mechanics::Simulate(float time)
{
	Start();
	Solve();
	Apply(time);
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

		if(r != 0)
			force += -mSpringConstant * (springVec / r) * (r - mSpringLength);

		force += -(spring.mMass1->mBody.mVelocity - spring.mMass2->mBody.mVelocity) * mSpringFriction;

		spring.mMass1->mBody.mForce += force;
		spring.mMass2->mBody.mForce += -force;		
	}
	
	//Apply Gravitational forces
	for(int i = 0; i < mRopeNodes.size(); ++i)
	{
		Mass& mass = mRopeNodes[i].mBody;
		mass.mForce += mGravitationalForce * mass.mMass;
		mass.mForce += -mAirFriction * mass.mVelocity;
	}
}

void Mechanics::Apply(float time)
{
	//float deltaTime = Time::Instance().deltaTime;
	float deltaTime = time;
	for(int i = 0; i < mRopeNodes.size(); ++i)
	{
		RopeNode& node = mRopeNodes[i];
		Mass& mass = node.mBody;

		// Euler
		/*mass.mVelocity += (mass.mForce/mass.mMass) * time;
		
		node.mPosition += mass.mVelocity * time;*/

		// RK4		
		/*State state;
		state.mPosition = node.mPosition;
		state.mVelocity = mass.mVelocity;
		
		glm::vec3 acceleration = mass.mForce / mass.mMass;
		Derivative kv1 = Func(state, 0.0f,Derivative(),acceleration);
		kv1.mDPosition /= 2.0f; kv1.mDVelocity /= 2.0f;
		Derivative kv2 = Func(state, deltaTime / 2.0f, kv1 ,acceleration);
		kv2.mDPosition /= 2.0f; kv2.mDVelocity /= 2.0f;
		Derivative kv3 = Func(state, deltaTime / 2.0f, kv2 ,acceleration);
		Derivative kv4 = Func(state, deltaTime, kv3,acceleration);
		
		mass.mVelocity += 1 / 6.0f * (kv1.mDVelocity + 2.0f * kv2.mDVelocity + 2.0f * kv3.mDVelocity + kv4.mDVelocity) * deltaTime;
		node.mPosition += 1 / 6.0f * (kv1.mDPosition + 2.0f * kv2.mDPosition + 2.0f * kv3.mDPosition + kv4.mDPosition) * deltaTime;*/
		
		glm::vec3 accleration = mass.mForce / mass.mMass;
		glm::vec3 kv1 = deltaTime * Func(accleration, 0.0f,accleration);
		glm::vec3 kv2 = deltaTime * Func(accleration, deltaTime / 2.0f, kv1 / 2.0f); // accl + accle/2
		glm::vec3 kv3 = deltaTime * Func(accleration, deltaTime / 2.0f, kv2 / 2.0f);
		glm::vec3 kv4 = deltaTime * Func(accleration, deltaTime, kv3);
		
		mass.mVelocity += 1 / 6.0f * (kv1 + 2.0f * kv2 + 2.0f * kv3 + kv4);
		
		glm::vec3 kp1 = deltaTime * Func(mass.mVelocity, 0.0f, mass.mVelocity);
		glm::vec3 kp2 = deltaTime * Func(mass.mVelocity, deltaTime / 2.0f, kp1 / 2.0f);
		glm::vec3 kp3 = deltaTime * Func(mass.mVelocity, deltaTime / 2.0f, kp2 / 2.0f);
		glm::vec3 kp4 = deltaTime * Func(mass.mVelocity, deltaTime, kp3);
		
		node.mPosition += 1 / 6.0f * (kp1 + 2.0f * kp2 + 2.0f * kp3 + kp4);
	}

	mRopeNodes[0].mPosition = mRopeConnectionPosition1;
	mRopeNodes[0].mBody.mVelocity = glm::vec3(0.0f);

	mRopeNodes[mRopeNodes.size() - 1].mPosition = mRopeConnectionPosition2;
	mRopeNodes[mRopeNodes.size() - 1].mBody.mVelocity = glm::vec3(0.0f);
}

Derivative Mechanics::Func(const State& initial, float time, const Derivative& x, glm::vec3 acceleration)
{
	State state;
	state.mPosition = initial.mPosition + x.mDPosition * time;
	state.mVelocity = initial.mVelocity + x.mDVelocity * time;

	Derivative output;
	output.mDPosition = state.mVelocity;
	output.mDVelocity = acceleration;
	return output;
}

glm::vec3 Mechanics::Func(glm::vec3 initial, float time, glm::vec3 x)
{
	return initial + x * time;
}
