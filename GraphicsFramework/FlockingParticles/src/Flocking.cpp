#include "Flocking.h"

#include "core/Engine.h"
#include "core/Camera.h"
#include "core/Light.h"
#include "core/Object.h"
#include "core/Time.h"
#include "core/ObjectManager.h"
#include "core/Components/Transform.h"
#include "core/Components/Material.h"
#include "opengl/Shader.h"
#include "opengl/Renderer.h"
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

	Renderer::Instance().SetClearColor(glm::vec3(0.461305, 0.627375, 0.627375));
	
	Engine::Instance().pCamera->mCameraPos = glm::vec3(-34.0f, 20.0f, 60.0f);
	Engine::Instance().pCamera->pitch = -19.0f;
	Engine::Instance().pCamera->yaw = -58.4f;	
	Engine::Instance().pCamera->mSpeed *= 2;
	Engine::Instance().pCamera->CalculateFront();

	mShader = new Shader("res/shaders/Drawing.vert", "res/shaders/Drawing.frag");

	mAvoidDistance = 2.0f;
	
	Boundary = ObjectManager::Instance().AddObject("res/data/Boundary.json");
	AddBoxObstacle(Boundary->GetComponent<Transform>(), mAvoidDistance, false);
		
	/*Object* box = ObjectManager::Instance().AddObject("res/data/Box.json");	
	Transform* btrans = box->GetComponent<Transform>();
	btrans->mPosition = glm::vec3(10.0f, 0.0f, 0.0f);
	AddBoxObstacle(btrans, 0.5f);
	box = ObjectManager::Instance().AddObject("res/data/Box.json");
	btrans = box->GetComponent<Transform>();
	btrans->mPosition = glm::vec3(-10.0f, 0.0f, 0.0f);
	AddBoxObstacle(btrans, 0.5f);*/
	
	SphereObstacle = ObjectManager::Instance().AddObject("res/data/Obstacle.json");
	Transform* trans = SphereObstacle->GetComponent<Transform>();
	trans->mPosition = glm::vec3(10.0f, 10.0f, 10.0f);
	mObstacles.push_back(new Sphere(trans->mPosition, trans->mScale.x, mAvoidDistance));

	SphereObstacle = ObjectManager::Instance().AddObject("res/data/Obstacle.json");
	trans = SphereObstacle->GetComponent<Transform>();
	trans->mPosition = glm::vec3(-10.0f, -10.0f, -10.0f);
	mObstacles.push_back(new Sphere(trans->mPosition, trans->mScale.x, mAvoidDistance));

	SphereObstacle = ObjectManager::Instance().AddObject("res/data/Obstacle.json");
	trans = SphereObstacle->GetComponent<Transform>();
	trans->mPosition = glm::vec3(10.0f, 10.0f, -10.0f);
	mObstacles.push_back(new Sphere(trans->mPosition, trans->mScale.x, mAvoidDistance));

	SphereObstacle = ObjectManager::Instance().AddObject("res/data/Obstacle.json");
	trans = SphereObstacle->GetComponent<Transform>();
	trans->mPosition = glm::vec3(-10.0f, -10.0f, 10.0f);
	mObstacles.push_back(new Sphere(trans->mPosition, trans->mScale.x, mAvoidDistance));

	SphereObstacle = ObjectManager::Instance().AddObject("res/data/Obstacle.json");
	trans = SphereObstacle->GetComponent<Transform>();
	trans->mPosition = glm::vec3(-10.0f, 10.0f, -10.0f);
	mObstacles.push_back(new Sphere(trans->mPosition, trans->mScale.x, mAvoidDistance));
	
	SphereObstacle = ObjectManager::Instance().AddObject("res/data/Obstacle.json");
	trans = SphereObstacle->GetComponent<Transform>();
	trans->mPosition = glm::vec3(10.0f, -10.0f, 10.0f);
	mObstacles.push_back(new Sphere(trans->mPosition, trans->mScale.x, mAvoidDistance));

	SphereObstacle = ObjectManager::Instance().AddObject("res/data/Obstacle.json");
	trans = SphereObstacle->GetComponent<Transform>();
	trans->mPosition = glm::vec3(10.0f, -10.0f, -10.0f);
	mObstacles.push_back(new Sphere(trans->mPosition, trans->mScale.x, mAvoidDistance));

	SphereObstacle = ObjectManager::Instance().AddObject("res/data/Obstacle.json");
	trans = SphereObstacle->GetComponent<Transform>();
	trans->mPosition = glm::vec3(-10.0f, 10.0f, 10.0f);
	mObstacles.push_back(new Sphere(trans->mPosition, trans->mScale.x, mAvoidDistance));

	//PARSE_JSON_FILE("res/data/FlockingSettings.json");
	mFishViewAngle = 45.0f * PI / 180.0f;
	mFishViewDistance = 10.0f;
	mBoidTightness = 2.5f;
	mMaxForce = 1.0f;
	mMaxAcceleration = 8.0f;
	mVelocityAttainTime = 0.75f;
	mMaxSpeed = 7.0f;
	mMinSpeed = 4.0f;

	mAvoidObstacleWeight = 20;
	mAlignWeight = 2;
	mCohesionWeight = 1;
	mSeparateWeight = 5.5f;
	mFPS = 0;
	Time::Instance().SetFrameRate(mFPS);
	
	auto rand = Random::Range(-1.0f, 1.0f);
	auto rand2 = Random::Range();
	glm::vec3 Y(0.0f, 1.0f, 0.0f);
	for(int i = 0; i < 250; ++i)
	{
		Object* fish = ObjectManager::Instance().AddObject("res/data/Fish.json");
		Flock* flock = fish->GetComponent<Flock>();
		Transform* transform = fish->GetComponent<Transform>();
		Material* material = fish->GetComponent<Material>();
		flock->mFront = glm::normalize(glm::vec3(rand(), rand(), rand()));
		transform->mPosition = glm::vec3(rand(), rand(), rand()) * 15.0f;
		glm::vec3 Y(0.0f, 1.0f, 0.0f);
		flock->mNormal = glm::normalize(glm::cross(Y, flock->mFront));
		flock->mMaxSpeed = mMaxSpeed;
		flock->mMinSpeed = mMinSpeed;
		flock->mVelocity = flock->mFront * (mMinSpeed + mMaxSpeed) / 2.0f;
		material->mDiffuse = glm::vec3(rand2(), rand2(), rand2());
		mFishes.push_back(fish);
	}
	
	ObjectManager::Instance().AddObject("res/data/Floor.json");
	Engine::Instance().mPause = false;
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
	
	ObjectManager::Instance().UpdateObjects();
	Renderer::Instance().DrawAllObjects(mShader);
	if (!Engine::Instance().mPause)
	{
#pragma omp parallel for schedule(dynamic, 1) // Magic: Multi-thread y loop
		for (auto fish : mFishes)
		{
			Flock* flock = fish->GetComponent<Flock>();
			glm::vec3 accels[4] = { glm::vec3(0.0f),glm::vec3(0.0f) ,glm::vec3(0.0f), glm::vec3(0.0f) };

			std::vector<Object*> neighbours = FindNeighbours(fish);
			AvoidObstacles(accels[0], fish);
			flock->mAcceleration += accels[0];
			if (neighbours.size() > 0)
			{
				NeighboursData& data = flock->mNeighboursData;
				data.mFlockCenter /= neighbours.size();
				accels[1] = SteerTowards(data.mFlockHeading,fish) * mAlignWeight;
				glm::vec3 offsetToCenter = data.mFlockCenter - fish->GetComponent<Transform>()->mPosition;
				accels[2] = SteerTowards(offsetToCenter, fish) * mCohesionWeight;

				accels[3] = SteerTowards(data.mAvoidanceDirection, fish) * mSeparateWeight;

				flock->mAcceleration += accels[1];
				flock->mAcceleration += accels[2];
				flock->mAcceleration += accels[3];
			}
		}
	}
}

void Flocking::DebugDisplay()
{
	ImGui::Checkbox("Pause Simulation", &Engine::Instance().mPause);
	ImGui::InputInt("FrameRate", &mFPS);
	if(ImGui::Button("Set FrameRate"))
	{
		Time::Instance().SetFrameRate(mFPS);
	}
	if(ImGui::Button("Reset FrameRate"))
	{
		Time::Instance().SetFrameRate(60);
	}
	ImGui::InputFloat("Max Acceleration", &mMaxAcceleration, 0.01f, 0.1f);
	if(ImGui::InputFloat("Minimum Velocity", &mMinSpeed, 0.2f, 1.0f)
		|| ImGui::InputFloat("Maximum Velocity", &mMaxSpeed, 0.2f, 1.0f))
	{
		for(auto fish : mFishes)
		{
			Flock* f = fish->GetComponent<Flock>();
			f->mMinSpeed = mMinSpeed;
			f->mMaxSpeed = mMaxSpeed;
		}
	}
	ImGui::InputFloat("Fish View Angle", &mFishViewAngle, 0.01f, 0.1f);
	ImGui::InputFloat("Fish View Distance", &mFishViewDistance, 0.01f, 0.1f);
	ImGui::InputFloat("Boid Tightness", &mBoidTightness, 0.01f, 0.1f);
	if(ImGui::InputFloat("Obstacle Avoidance Distance",&mAvoidDistance,0.5f,1.0f))
	{
		for(auto obs : mObstacles)
		{
			obs->mDistanceThreshold = mAvoidDistance;
		}
	}
}

std::vector<Object*> Flocking::FindNeighbours(Object* fish)
{
	std::vector<Object*> list;
	glm::vec3 posA = fish->GetComponent<Transform>()->mPosition;
	glm::vec3 velA = fish->GetComponent<Flock>()->mVelocity;
	NeighboursData& data = fish->GetComponent<Flock>()->mNeighboursData;
	data.Clear();
	for(auto obj : mFishes)
	{
		if(obj != fish)
		{
			glm::vec3 posB = obj->GetComponent<Transform>()->mPosition;

			glm::vec3 P = glm::normalize(posB - posA);
			glm::vec3 V = glm::normalize(velA);
			float dot = glm::dot(P,V);
			float angle = glm::acos(fabs(dot));
			float dist = glm::length(posB - posA);
			if (angle <= mFishViewAngle && dist <= mFishViewDistance)
			//if(dist < mFishViewDistance)
			{
				list.push_back(obj);			
			}
		}
	}
	if(list.size() > 8)
	{
		std::sort(list.begin(), list.end(), [posA](Object* a, Object* b)
			{
				glm::vec3 pos_a = a->GetComponent<Transform>()->mPosition;
				glm::vec3 pos_b = b->GetComponent<Transform>()->mPosition;

				return glm::length(pos_a - posA) < glm::length(pos_b - posA);
			});
		list.erase(list.begin() + 5, list.end());
	}

	for(auto n : list)
	{
		Flock* fo = n->GetComponent<Flock>();
		glm::vec3 posB = n->GetComponent<Transform>()->mPosition;
		float dist = glm::length(posB - posA);
		data.mFlockHeading += fo->mFront;
		data.mFlockCenter += posB;

		if (dist < mBoidTightness)
		{
			data.mAvoidanceDirection += glm::normalize(posA - posB);
		}
	}
	return list;
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
			//a_first += (glm::length(V) * u - V) / mVelocityAttainTime;
			a_first += SteerTowards(u,fish) * mAvoidObstacleWeight;
		}
	}
}

glm::vec3 Flocking::SteerTowards(glm::vec3& vector, Object* fish)
{
	float len = glm::length(vector);
	if(len == 0.0f)
	{
		return vector;
	}
	glm::vec3 v = vector/len * mMaxSpeed - fish->GetComponent<Flock>()->mVelocity;
	float vlen = glm::length(v);
	if(vlen > mMaxForce)
	{
		v = v/vlen * mMaxForce;
	}
	return v;
}

void Flocking::AddBoxObstacle(Transform* transform,float avoidDistance, bool outwardNormals)
{
	glm::vec3 scale = transform->mScale / 2.0f;

	float sign = outwardNormals ? -1.0f : 1.0f;
	
	mObstacles.push_back(new Wall(transform->mPosition + glm::vec3(scale.x, 0.0f, 0.0f), 
		sign * glm::vec3(-1.0f, 0.0f, 0.0f), 	avoidDistance,!outwardNormals,
		outwardNormals? glm::vec3(0.0f,scale.y,scale.z): glm::vec3(std::numeric_limits<float>::max())));
	
	mObstacles.push_back(new Wall(transform->mPosition + glm::vec3(-scale.x, 0.0f, 0.0f), 
		sign * glm::vec3(1.0f, 0.0f, 0.0f), avoidDistance, !outwardNormals,
		outwardNormals ? glm::vec3(0.0f, scale.y, scale.z) : glm::vec3(std::numeric_limits<float>::max())));
	
	mObstacles.push_back(new Wall(transform->mPosition + glm::vec3(0.0f, scale.y, 0.0f),
		sign * glm::vec3(0.0f, -1.0f, 0.0f), avoidDistance, !outwardNormals,
		outwardNormals ? glm::vec3(scale.x, 0.0f, scale.z) : glm::vec3(std::numeric_limits<float>::max())));
	
	mObstacles.push_back(new Wall(transform->mPosition + glm::vec3(0.0f, -scale.y, 0.0f), 
		sign * glm::vec3(0.0f, 1.0f, 0.0f), avoidDistance, !outwardNormals,
		outwardNormals ? glm::vec3(scale.x, 0.0f, scale.z) : glm::vec3(std::numeric_limits<float>::max())));
	
	mObstacles.push_back(new Wall(transform->mPosition + glm::vec3(0.0f, 0.0f, scale.z), 
		sign * glm::vec3(0.0f, 0.0f, -1.0f), avoidDistance, !outwardNormals,
		outwardNormals ? glm::vec3(scale.x, scale.y, 0.0f) : glm::vec3(std::numeric_limits<float>::max())));
	
	mObstacles.push_back(new Wall(transform->mPosition + glm::vec3(0.0f, 0.0f, -scale.z), 
		sign * glm::vec3(0.0f, 0.0f, 1.0f), avoidDistance, !outwardNormals,
		outwardNormals ? glm::vec3(scale.x, scale.y, 0.0f) : glm::vec3(0.0f)));

}
