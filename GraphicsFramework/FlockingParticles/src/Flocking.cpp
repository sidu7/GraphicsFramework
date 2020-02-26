#include "Flocking.h"

#include "core/Engine.h"
#include "core/Camera.h"
#include "core/Light.h"
#include "core/Object.h"
#include "core/ObjectManager.h"
#include "core/Components/Transform.h"
#include "core/Components/Material.h"
#include "opengl/Shader.h"
#include "utils/Random.h"
#include "utils/JSONHelper.h"

#include "Flock.h"
#include <glm/gtx/norm.inl>
#include <Imgui/imgui.h>
#include "Obstacle.h"

const float PI = 22.0f / 7.0f;

Flocking::~Flocking()
{
	std::for_each(mObstacles.begin(), mObstacles.end(), [](Obstacle* x) { delete x; });
}

void Flocking::Init()
{
	mLight = new Light(glm::vec3(10.0, 100.0, 40.0));

	Engine::Instance().pCamera->mCameraPos = glm::vec3(-34.0f, 20.0f, 60.0f);
	Engine::Instance().pCamera->pitch = -19.0f;
	Engine::Instance().pCamera->yaw = -58.4f;	
	Engine::Instance().pCamera->mSpeed *= 2;
	Engine::Instance().pCamera->CalculateFront();

	mShader = new Shader("res/shaders/Drawing.vert", "res/shaders/Drawing.frag");

	Boundary = ObjectManager::Instance().AddObject("res/data/Boundary.json");
	Object* fish = ObjectManager::Instance().AddObject("res/data/Fish.json");
	Flock* flock = fish->GetComponent<Flock>();
	flock->mVelocity = glm::normalize(glm::vec3(2.0f, 1.0f, 0.0f));
	glm::vec3 Y(0.0f, 1.0f, 0.0f);
	flock->mNormal = glm::normalize(glm::cross(flock->mVelocity,Y));
	mFishes.push_back(fish);
		
	float scale = 30.0f/2;

	mObstacles.push_back(new Wall(glm::vec3(15.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), 5.0f));
	mObstacles.push_back(new Wall(glm::vec3(-15.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), 5.0f));
	mObstacles.push_back(new Wall(glm::vec3(0.0f, 15.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), 5.0f));
	mObstacles.push_back(new Wall(glm::vec3(0.0f, -15.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), 5.0f));
	mObstacles.push_back(new Wall(glm::vec3(0.0f, 0.0f, 15.0f), glm::vec3(0.0f, 0.0f, -1.0f), 5.0f));
	mObstacles.push_back(new Wall(glm::vec3(0.0f, 0.0f, -15.0f), glm::vec3(0.0f, 0.0f, 1.0f), 5.0f));

	//PARSE_JSON_FILE("res/data/FlockingSettings.json");
	mFishViewAngle = 60.0f * PI / 180.0f;
	mFishViewDistance = 10.0f;
	mBoidTightness = 2.5f;
	mMaxAcceleration = 15.3f;
	mVelocityAttainTime = 1.0f;
	return;
	auto rand = Random::Range(-1.0f, 1.0f);
	auto rand2 = Random::Range();
	//glm::vec3 Y(0.0f, 1.0f, 0.0f);
	for(int i = 0; i < 20; ++i)
	{
		Object* fish = ObjectManager::Instance().AddObject("res/data/Fish.json");
		Flock* flock = fish->GetComponent<Flock>();
		Transform* transform = fish->GetComponent<Transform>();
		Material* material = fish->GetComponent<Material>();
		flock->mNormal = glm::normalize(glm::vec3(rand(),rand(),rand()));
		transform->mPosition = glm::vec3(rand(), rand(), rand()) * scale;
		flock->mVelocity = glm::cross(flock->mNormal, Y);
		material->mDiffuse = glm::vec3(rand2(), rand2(), rand2());
		mFishes.push_back(fish);
	}
	
	//ObjectManager::Instance().AddObject("res/data/Floor.json");
}

void Flocking::Update()
{
	
	mShader->Bind();
	mShader->SetUniformMat4f("view", Engine::Instance().pCamera->mView);
	mShader->SetUniformMat4f("prevview", Engine::Instance().pCamera->mPrevView);
	mShader->SetUniformMat4f("projection", Engine::Instance().pCamera->mProjection);
	mShader->SetUniformMat4f("inverseview", glm::inverse(Engine::Instance().pCamera->mView));
	mShader->SetUniform3f("lightPos", mLight->position);
	mShader->SetUniform3f("light", 3.2f, 3.2f, 3.2f);
	
	ObjectManager::Instance().ObjectsDraw(mShader);

	for (auto fish : mFishes)
	{
		Flock* flock = fish->GetComponent<Flock>();
		glm::vec3 accels[3] = { glm::vec3(0.0f),glm::vec3(0.0f) ,glm::vec3(0.0f) };
		//std::vector<Object*> neighbours = FindNeighbours(fish);
		//if (neighbours.size() > 0)
		{
			//FirstandThirdAcceleration(neighbours, fish->GetComponent<Transform>()->mPosition, accels[0], accels[2]);
			AvoidObstacles(accels[0],fish);
			//accels[1] = SecondAcceleration(neighbours, flock->mVelocity);
			flock->mAcceleration = PrioritizedAcceleration(accels);
		}
	}
}

void Flocking::DebugDisplay()
{
	ImGui::InputFloat("Max Acceleration", &mMaxAcceleration, 0.01f, 0.1f);
	ImGui::InputFloat("Fish View Angle", &mFishViewAngle, 0.01f, 0.1f);
	ImGui::InputFloat("Fish View Distance", &mFishViewDistance, 0.01f, 0.1f);
	ImGui::InputFloat("Boid Tightness", &mBoidTightness, 0.01f, 0.1f);
	ImGui::InputFloat("Velocity Attain Time", &mVelocityAttainTime, 0.01f,0.1f);
}

std::vector<Object*> Flocking::FindNeighbours(Object* fish)
{
	std::vector<Object*> list;
	glm::vec3 posA = fish->GetComponent<Transform>()->mPosition;
	glm::vec3 velA = fish->GetComponent<Flock>()->mVelocity;
	for(auto obj : mFishes)
	{
		if(obj != fish)
		{
			glm::vec3 posB = obj->GetComponent<Transform>()->mPosition;

			glm::vec3 P = glm::normalize(posB - posA);
			glm::vec3 V = glm::normalize(velA);
			float dot = glm::dot(P,V);
			float angle = glm::acos(dot);
			float dist = glm::length(posB - posA);
			if (angle <= mFishViewAngle && dist <= mFishViewDistance)
			//if(dist <= mFishViewDistance)
			{
				list.push_back(obj);
			}
		}
	}
	if(list.size() > 5)
	{
		std::sort(list.begin(), list.end(), [posA](Object* a, Object* b)
			{
				glm::vec3 pos_a = a->GetComponent<Transform>()->mPosition;
				glm::vec3 pos_b = b->GetComponent<Transform>()->mPosition;

				return glm::length(pos_a - posA) < glm::length(pos_b - posA);
			});
		list.erase(list.begin() + 5, list.end());
	}
	return list;
}

void Flocking::FirstandThirdAcceleration(std::vector<Object*>& neighbours, glm::vec3& posA, 
	glm::vec3& a_first, glm::vec3& a_third)
{
	for (auto n : neighbours)
	{
		glm::vec3 posB = n->GetComponent<Transform>()->mPosition;
		glm::vec3 vec = posB - posA;
		float gamma_b = (mBoidTightness * mBoidTightness / glm::length2(vec)) - 1;

		if (gamma_b < 0.0f) // attractive force
		{
			a_first += glm::vec3(0.0f);
			a_third += -mMaxAcceleration * gamma_b * (vec) / glm::length(vec);
		}
		else // replusive force
		{
			a_third += glm::vec3(0.0f);
			a_first += -mMaxAcceleration * gamma_b * (vec) / glm::length(vec);
		}
	}
}

void Flocking::AvoidObstacles(glm::vec3& a_first, Object* fish)
{
	glm::vec3 P = fish->GetComponent<Transform>()->mPosition;
	glm::vec3 V = fish->GetComponent<Flock>()->mVelocity;
	glm::vec3 M = fish->GetComponent<Flock>()->mNormal;
	for(auto obstacle : mObstacles)
	{
		if(obstacle->WillCollide(P,V))
		{
			glm::vec3 u = obstacle->AvoidanceDirection(P, V, M);
			a_first += (glm::length(V) * u - V) / mVelocityAttainTime;
		}
	}
}

glm::vec3 Flocking::SecondAcceleration(std::vector<Object*>& neighbours, glm::vec3 velA)
{
	glm::vec3 velAvg(0.0f);

	for(auto n : neighbours)
	{
		velAvg += n->GetComponent<Flock>()->mVelocity;
	}
	velAvg /= neighbours.size();

	return (velAvg - velA) / mVelocityAttainTime;
}

glm::vec3 Flocking::PrioritizedAcceleration(const glm::vec3* accels)
{
	glm::vec3 netAccel = glm::vec3(0.0f);
	int i = 0;
	while (i < 3 && glm::length(netAccel) < mMaxAcceleration)
	{
		netAccel = netAccel + accels[i];
		++i;
	}

	if (glm::length(netAccel) > mMaxAcceleration)
	{
		float l_Anet = glm::length(netAccel);
		float l_Ai_1 = glm::length(accels[i - 1]);
		float dot_product = glm::dot(netAccel, accels[i - 1]);
		float x = (dot_product - glm::sqrt(l_Ai_1 * l_Ai_1 * mMaxAcceleration * mMaxAcceleration -
			l_Anet * l_Anet * l_Ai_1 * l_Ai_1 +
			dot_product * dot_product)) / (l_Ai_1 * l_Ai_1);
		netAccel = netAccel - x * accels[i - 1];
	}

	return netAccel;
}
