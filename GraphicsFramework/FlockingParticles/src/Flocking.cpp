#include "Flocking.h"

#include "Core/Engine.h"
#include "Core/Camera.h"
#include "Core/Light.h"
#include "Core/Object.h"
#include "Core/ShapeManager.h"
#include "Core/Time.h"
#include "Core/ObjectManager.h"
#include "Core/Components/Transform.h"
#include "Core/Components/Material.h"
#include "Core/Components/Shape.h"
#include "Core/Profiling/Profiler.h"
#include "Rendering/Shader.h"
#include "Rendering/Renderer.h"
#include "Rendering/UniformBuffer.h"
#include "Rendering/ShaderRegistry.h"
#include "Utils/Random.h"
#include "Utils/JSONHelper.h"

#include "Flock.h"
#include <Imgui/imgui.h>
#include "Obstacle.h"

const float PI = 22.0f / 7.0f;

// Binding points consumed by res/shaders/Drawing.vert / .frag. Transform (0) and
// Material (1) come from Settings.json; the scene owns the camera and light blocks.
constexpr uint32_t CameraBindingPoint = 2;
constexpr uint32_t LightBindingPoint = 3;

struct CameraMatricesUBO
{
	glm::mat4 mProjection;
	glm::mat4 mView;
	glm::mat4 mInverseView;
};

struct LightDataUBO
{
	glm::vec4 mPosition;
	glm::vec4 mColor;
};

// FlockingParticles project-local shader IDs
enum class FlockingShaderID : int32_t
{
	NONE = 0,
	DRAWING,
};

Flocking::~Flocking()
{
	std::for_each(mObstacles.begin(), mObstacles.end(), [](Obstacle* x) { delete x; });
}

void Flocking::Close()
{
	delete mLight;
	delete mShader;
	delete mCameraUBO;
	delete mLightUBO;
}

void Flocking::Init()
{
	// Initialize shader registry before creating shaders
	ShaderRegistry::Instance()->Init("res/shaders/", [](ShaderRegistry* reg) {
		reg->RegisterShader(static_cast<int32_t>(FlockingShaderID::DRAWING),
							"Drawing.vert", "Drawing.frag",
							"spirv/Drawing.vert.spv", "spirv/Drawing.frag.spv");
	});

	mLight = new Light(glm::vec3(10.0, 100.0, 40.0));

	Renderer::Instance()->SetClearColor(glm::vec3(0.461305, 0.627375, 0.627375));
	
	Engine::Instance()->GetCamera()->mCameraPos = glm::vec3(-34.0f, 20.0f, 60.0f);
	Engine::Instance()->GetCamera()->pitch = -19.0f;
	Engine::Instance()->GetCamera()->yaw = -58.4f;	
	Engine::Instance()->GetCamera()->mSpeed *= 2;
	Engine::Instance()->GetCamera()->CalculateFront();

	mShader = Renderer::Instance()->CreateShader();
	mShader->Init(static_cast<int32_t>(FlockingShaderID::DRAWING));

	// Per-frame uniform blocks read by Drawing.vert / Drawing.frag
	mCameraUBO = Renderer::Instance()->CreateUniformBuffer();
	mCameraUBO->Init(sizeof(CameraMatricesUBO), CameraBindingPoint);

	mLightUBO = Renderer::Instance()->CreateUniformBuffer();
	mLightUBO->Init(sizeof(LightDataUBO), LightBindingPoint);

	mAvoidDistance = 2.0f;

	// Created first so it is drawn before the flock and obstacles. Vulkan has no
	// depth attachment yet, so draw order is the only ordering guarantee there.
	ObjectManager::Instance()->AddObject("res/data/Floor.json");

	Boundary = ObjectManager::Instance()->AddObject("res/data/Boundary.json");
	AddBoxObstacle(Boundary->GetComponent<Transform>(), mAvoidDistance, false);
		
	/*Object* box = ObjectManager::Instance()->AddObject("res/data/Box.json");	
	Transform* btrans = box->GetComponent<Transform>();
	btrans->mPosition = glm::vec3(10.0f, 0.0f, 0.0f);
	AddBoxObstacle(btrans, 0.5f);
	box = ObjectManager::Instance()->AddObject("res/data/Box.json");
	btrans = box->GetComponent<Transform>();
	btrans->mPosition = glm::vec3(-10.0f, 0.0f, 0.0f);
	AddBoxObstacle(btrans, 0.5f);*/
	
	SphereObstacle = ObjectManager::Instance()->AddObject("res/data/Obstacle.json");
	Transform* trans = SphereObstacle->GetComponent<Transform>();
	trans->mPosition = glm::vec3(10.0f, 10.0f, 10.0f);
	mObstacles.push_back(new Sphere(trans->mPosition, trans->mScale.x, mAvoidDistance));

	SphereObstacle = ObjectManager::Instance()->AddObject("res/data/Obstacle.json");
	trans = SphereObstacle->GetComponent<Transform>();
	trans->mPosition = glm::vec3(-10.0f, -10.0f, -10.0f);
	mObstacles.push_back(new Sphere(trans->mPosition, trans->mScale.x, mAvoidDistance));

	SphereObstacle = ObjectManager::Instance()->AddObject("res/data/Obstacle.json");
	trans = SphereObstacle->GetComponent<Transform>();
	trans->mPosition = glm::vec3(10.0f, 10.0f, -10.0f);
	mObstacles.push_back(new Sphere(trans->mPosition, trans->mScale.x, mAvoidDistance));

	SphereObstacle = ObjectManager::Instance()->AddObject("res/data/Obstacle.json");
	trans = SphereObstacle->GetComponent<Transform>();
	trans->mPosition = glm::vec3(-10.0f, -10.0f, 10.0f);
	mObstacles.push_back(new Sphere(trans->mPosition, trans->mScale.x, mAvoidDistance));

	SphereObstacle = ObjectManager::Instance()->AddObject("res/data/Obstacle.json");
	trans = SphereObstacle->GetComponent<Transform>();
	trans->mPosition = glm::vec3(-10.0f, 10.0f, -10.0f);
	mObstacles.push_back(new Sphere(trans->mPosition, trans->mScale.x, mAvoidDistance));
	
	SphereObstacle = ObjectManager::Instance()->AddObject("res/data/Obstacle.json");
	trans = SphereObstacle->GetComponent<Transform>();
	trans->mPosition = glm::vec3(10.0f, -10.0f, 10.0f);
	mObstacles.push_back(new Sphere(trans->mPosition, trans->mScale.x, mAvoidDistance));

	SphereObstacle = ObjectManager::Instance()->AddObject("res/data/Obstacle.json");
	trans = SphereObstacle->GetComponent<Transform>();
	trans->mPosition = glm::vec3(10.0f, -10.0f, -10.0f);
	mObstacles.push_back(new Sphere(trans->mPosition, trans->mScale.x, mAvoidDistance));

	SphereObstacle = ObjectManager::Instance()->AddObject("res/data/Obstacle.json");
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
	mFPS = 120;  // Default to 120 FPS (unlimited when neededTime=0, capped by display otherwise)
	Time::Instance()->SetFrameRate(mFPS);

	// Pre-allocate neighbor buffer — avoids 250 heap allocations per frame
	// in the parallel update loop (see FindNeighbours, line ~214).
	mNeighbourBuffer.reserve(mFishes.size() * 2);

	auto rand = Random::Range(-1.0f, 1.0f);
	auto rand2 = Random::Range();
	glm::vec3 Y(0.0f, 1.0f, 0.0f);
	for(int i = 0; i < 250; ++i)
	{
		Object* fish = ObjectManager::Instance()->AddObject("res/data/Fish.json");
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
	
	Engine::Instance()->mPause = false;
}

void Flocking::Update()
{	
	GF_PROFILE_SCOPE("Flocking::Update");

	Camera* camera = Engine::Instance()->GetCamera();
	CameraMatricesUBO cameraData{ camera->mProjection, camera->mView, glm::inverse(camera->mView) };
	mCameraUBO->AddData(sizeof(cameraData), &cameraData);

	LightDataUBO lightData{ glm::vec4(mLight->position, 1.0f), glm::vec4(1.0f) };
	mLightUBO->AddData(sizeof(lightData), &lightData);

	Renderer::Instance()->BindUniformBuffer(mCameraUBO, CameraBindingPoint);
	Renderer::Instance()->BindUniformBuffer(mLightUBO, LightBindingPoint);

	{
		GF_PROFILE_SCOPE("Flocking::Update - ObjectManager::UpdateObjects");
		ObjectManager::Instance()->UpdateObjects();
	}
	ObjectManager::Instance()->RenderObjects(mShader);
	if (!Engine::Instance()->mPause)
	{
		GF_PROFILE_SCOPE("Flocking::Update - UpdateFishes");
		for (auto fish : mFishes)
		{
			Flock* flock = fish->GetComponent<Flock>();
			glm::vec3 accels[4] = { glm::vec3(0.0f),glm::vec3(0.0f) ,glm::vec3(0.0f), glm::vec3(0.0f) };

			// Reuse the pre-allocated buffer to avoid heap allocation every frame.
			mNeighbourBuffer.clear();
			FindNeighbours(fish, mNeighbourBuffer);
			auto& neighbours = mNeighbourBuffer;
			AvoidObstacles(accels[0], fish);
			flock->mAcceleration += accels[0];
			if (!neighbours.empty())
			{
				NeighboursData& data = flock->mNeighboursData;
				data.mFlockCenter /= static_cast<float>(neighbours.size());
				accels[1] = SteerTowards(data.mFlockHeading, fish) * mAlignWeight;
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

void Flocking::RenderObject(Object* object, Shader* shader)
{
	Transform* transform = object->GetComponent<Transform>();
	if (transform && transform->mMatricesUBO)
	{
		Renderer::Instance()->BindUniformBuffer(transform->mMatricesUBO, transform->mMatricesUBO->GetBinding());
	}

	Material* material = object->GetComponent<Material>();
	if (material && material->mMaterialUBO)
	{
		Renderer::Instance()->BindUniformBuffer(material->mMaterialUBO, material->mMaterialUBO->GetBinding());
	}

	Shape* shape = object->GetComponent<Shape>();
	if (shape)
	{
		if (shape->mMesh)
		{
			Renderer::Instance()->Draw(shape->mShapeData->mVBO, shape->mShapeData->mIBO, shader);
		}
		if (shape->mWireMesh)
		{
			Renderer::Instance()->DebugDrawLines(shape->mShapeData->mVBO, shape->mShapeData->mIBO, shader);
		}
		if (shape->mDebugMesh)
		{
			Renderer::Instance()->DebugDraw(shape->mShapeData->mVBO, shape->mShapeData->mIBO, shader);
		}
	}

	if (transform)
	{
		Renderer::Instance()->UnbindUniformBuffer(transform->mMatricesUBO);
	}
	if (material)
	{
		Renderer::Instance()->UnbindUniformBuffer(material->mMaterialUBO);
	}
}

void Flocking::DebugDisplay()
{
	ImGui::SetNextWindowPos(ImVec2(330.0f, 40.0f), ImGuiCond_FirstUseEver);
	ImGui::Begin("Flocking");
	ImGui::Checkbox("Pause Simulation", &Engine::Instance()->mPause);
	ImGui::InputInt("FrameRate", &mFPS);
	if(ImGui::Button("Set FrameRate"))
	{
		Time::Instance()->SetFrameRate(mFPS);
	}
	if(ImGui::Button("Reset FrameRate"))
	{
		Time::Instance()->SetFrameRate(60);
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
	ImGui::End();
}

std::vector<Object*> Flocking::FindNeighbours(Object* fish, std::vector<Object*>& neighbourBuffer)
{
	neighbourBuffer.clear();
	glm::vec3 posA = fish->GetComponent<Transform>()->mPosition;
	NeighboursData& data = fish->GetComponent<Flock>()->mNeighboursData;
	data.Clear();

	// Normalize velocity once, outside the loop
	glm::vec3 velA = fish->GetComponent<Flock>()->mVelocity;
	const glm::vec3 normVel = glm::normalize(velA);

	for(auto obj : mFishes)
	{
		if(obj == fish) continue;

		glm::vec3 posB = obj->GetComponent<Transform>()->mPosition;
		glm::vec3 diff = posB - posA;
		float distSq = glm::dot(diff, diff);
		if (distSq > mFishViewDistance * mFishViewDistance) continue;

		// Angular check: is obj within the fish's viewing cone?
		float dot = glm::dot(glm::normalize(diff), normVel);
		if (dot >= 1.0f) {
			// Exactly ahead — always within cone
		} else {
			float angle = glm::acos(glm::clamp(dot, -1.0f, 1.0f));
			if (angle > mFishViewAngle) continue;
		}

		neighbourBuffer.push_back(obj);
	}

	// Keep only the 5 closest neighbors (sort by distance, keep first 5)
	const int kMaxNeighbors = 5;
	if (neighbourBuffer.size() > static_cast<size_t>(kMaxNeighbors))
	{
		std::partial_sort(neighbourBuffer.begin(),
		                  neighbourBuffer.begin() + kMaxNeighbors,
		                  neighbourBuffer.end(),
		                  [&posA](Object* a, Object* b) {
			                  return glm::dot(a->GetComponent<Transform>()->mPosition - posA, a->GetComponent<Transform>()->mPosition - posA)
	                             < glm::dot(b->GetComponent<Transform>()->mPosition - posA, b->GetComponent<Transform>()->mPosition - posA);
		                  });
		neighbourBuffer.resize(kMaxNeighbors);
	}

	// Aggregate neighbor data using only the kept neighbors
	for(auto n : neighbourBuffer)
	{
		Flock* fo = n->GetComponent<Flock>();
		glm::vec3 posB = n->GetComponent<Transform>()->mPosition;
		float distSq = glm::dot(posB - posA, posB - posA);
		data.mFlockHeading += fo->mFront;
		data.mFlockCenter += posB;

		if (distSq < mBoidTightness * mBoidTightness)
		{
			data.mAvoidanceDirection += glm::normalize(posA - posB);
		}
	}

	return neighbourBuffer;
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
		outwardNormals ? glm::vec3(scale.x, 0.0f, scale.z) : glm::vec3(0.0f)));
	
	mObstacles.push_back(new Wall(transform->mPosition + glm::vec3(0.0f, 0.0f, scale.z), 
		sign * glm::vec3(0.0f, 0.0f, -1.0f), avoidDistance, !outwardNormals,
		outwardNormals ? glm::vec3(scale.x, scale.y, 0.0f) : glm::vec3(0.0f)));
	
	mObstacles.push_back(new Wall(transform->mPosition + glm::vec3(0.0f, 0.0f, -scale.z), 
		sign * glm::vec3(0.0f, 0.0f, 1.0f), avoidDistance, !outwardNormals,
		outwardNormals ? glm::vec3(scale.x, scale.y, 0.0f) : glm::vec3(0.0f)));

}
