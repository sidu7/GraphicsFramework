#include "Mechanics.h"

// Application-local shader IDs (shared across all scenes in this project)
#include "ProjectShaderIDs.h"

#include "Core/Engine.h"
#include "Core/Camera.h"
#include "Core/Light.h"
#include "Core/Inputs.h"
#include "Core/Time.h"
#include "Core/ShapeManager.h"
#include "Rendering/Shader.h"
#include "Rendering/Renderer.h"
#include "Rendering/ShaderRegistry.h"
#include "Rendering/UniformBuffer.h"
#include "Core/Components/Transform.h"
#include "Core/Components/Material.h"

#include <Imgui/imgui.h>
#include <glm/gtc/matrix_transform.hpp>

namespace
{
	constexpr uint32_t ObjectBindingPoint = 0;
	constexpr uint32_t MaterialBindingPoint = 1;
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

	struct MaterialUBO
	{
		glm::vec4 mDiffuse;
		glm::vec4 mSpecular;
		float mShininess;
		int32_t mLighting;
		int32_t mUseTexture;
		int32_t mPad0;
		int32_t mPad1;
		int32_t mPad2;
	};
}

Mechanics::~Mechanics()
{
	Close();
}

void Mechanics::Init()
{
	mLight = new Light(glm::vec3(10.0f, 100.0f, 40.0f));
	mPauseSimulation = false;
	Time = 0.0f;
	NodeCount = 80;
	NodeMass = 0.05f;
	mSpringLength = 0.5f;
	mSpringConstant = 30.0f;
	mSpringFriction = 0.2f;
	mGravitationalForce = glm::vec3(0.0f, -9.8f, 0.0f);
	mAirFriction = 0.02f;
	mRopeConnectionPosition1 = glm::vec3(0.0f, 6.0f, 0.0f);
	mRopeConnectionPosition2 = glm::vec3(40.0f, 6.0f, 0.0f);
	mRopeConnectionVelocity = 50.0f;

	Engine::Instance()->GetCamera()->mCameraPos = glm::vec3(20.0f, 23.0f, 60.0f);
	Engine::Instance()->GetCamera()->pitch = -20.0f;
	Engine::Instance()->GetCamera()->CalculateFront();

	// Initialize shader registry
	ShaderRegistry::Instance()->Init("res/Shaders/", [](ShaderRegistry* reg) {
		reg->RegisterShader(static_cast<int32_t>(AnimationShaderID::MODEL_DRAW),
							"ModelDraw.vert", "ModelDraw.frag",
							"spirv/ModelDraw.vert.spv", "spirv/ModelDraw.frag.spv");
		reg->RegisterShader(static_cast<int32_t>(AnimationShaderID::DRAWING),
							"Drawing.vert", "Drawing.frag",
							"spirv/Drawing.vert.spv", "spirv/Drawing.frag.spv");
	});

	Drawing = Renderer::Instance()->CreateShader();
	Drawing->Init(static_cast<int32_t>(AnimationShaderID::DRAWING));

	// Create shared UBOs
	mCameraUBO = Renderer::Instance()->CreateUniformBuffer();
	mCameraUBO->Init(sizeof(CameraMatricesUBO), CameraBindingPoint);

	mLightUBO = Renderer::Instance()->CreateUniformBuffer();
	mLightUBO->Init(sizeof(LightDataUBO), LightBindingPoint);

	ResetNodesAndSprings();

	Renderer::Instance()->SetClearColor(glm::vec3(0.2f, 0.2f, 0.2f));
	Renderer::Instance()->SetDepthTest(true);
}

void Mechanics::Close()
{
	delete mLight;
	mLight = nullptr;

	delete Drawing;
	Drawing = nullptr;

	delete mCameraUBO;
	mCameraUBO = nullptr;

	delete mLightUBO;
	mLightUBO = nullptr;

	for (auto& entry : mNodeUBOs)
	{
		delete entry.mObjectUBO;
		delete entry.mMaterialUBO;
	}
	mNodeUBOs.clear();

	mSprings.clear();
	mRopeNodes.clear();
}

void Mechanics::RenderObject(Object* object, Shader* shader)
{
}

void Mechanics::Update()
{
	float deltaTime = Time::Instance()->deltaTime;

	Time += deltaTime;

	float maxDT = 0.016f;
	int numIterations = (int)(deltaTime / maxDT) + 1;

	if (numIterations != 0)
	{
		deltaTime = deltaTime / numIterations;
	}

	if (!mPauseSimulation)
	{
		for (int i = 0; i < numIterations; ++i)
		{
			Simulate(deltaTime);
		}
	}

	deltaTime = Time::Instance()->deltaTime;

	if (Inputs::Instance()->IsPressed(SDL_SCANCODE_LSHIFT))
	{
		if (Inputs::Instance()->IsPressed(SDL_SCANCODE_Y))
		{
			mRopeConnectionPosition1 += glm::vec3(0.0f, mRopeConnectionVelocity * deltaTime, 0.0f);
		}
		if (Inputs::Instance()->IsPressed(SDL_SCANCODE_H))
		{
			mRopeConnectionPosition1 -= glm::vec3(0.0f, mRopeConnectionVelocity * deltaTime, 0.0f);
		}
		if (Inputs::Instance()->IsPressed(SDL_SCANCODE_UP))
		{
			mRopeConnectionPosition2 += glm::vec3(0.0f, mRopeConnectionVelocity * deltaTime, 0.0f);
		}
		if (Inputs::Instance()->IsPressed(SDL_SCANCODE_DOWN))
		{
			mRopeConnectionPosition2 -= glm::vec3(0.0f, mRopeConnectionVelocity * deltaTime, 0.0f);
		}
	}
	else
	{
		if (Inputs::Instance()->IsPressed(SDL_SCANCODE_Y))
		{
			mRopeConnectionPosition1 -= glm::vec3(0.0f, 0.0f, mRopeConnectionVelocity * deltaTime);
		}
		if (Inputs::Instance()->IsPressed(SDL_SCANCODE_H))
		{
			mRopeConnectionPosition1 += glm::vec3(0.0f, 0.0f, mRopeConnectionVelocity * deltaTime);
		}
		if (Inputs::Instance()->IsPressed(SDL_SCANCODE_G))
		{
			mRopeConnectionPosition1 -= glm::vec3(mRopeConnectionVelocity * deltaTime, 0.0f, 0.0f);
		}
		if (Inputs::Instance()->IsPressed(SDL_SCANCODE_J))
		{
			mRopeConnectionPosition1 += glm::vec3(mRopeConnectionVelocity * deltaTime, 0.0f, 0.0f);
		}
		if (Inputs::Instance()->IsPressed(SDL_SCANCODE_UP))
		{
			mRopeConnectionPosition2 -= glm::vec3(0.0f, 0.0f, mRopeConnectionVelocity * deltaTime);
		}
		if (Inputs::Instance()->IsPressed(SDL_SCANCODE_DOWN))
		{
			mRopeConnectionPosition2 += glm::vec3(0.0f, 0.0f, mRopeConnectionVelocity * deltaTime);
		}
		if (Inputs::Instance()->IsPressed(SDL_SCANCODE_RIGHT))
		{
			mRopeConnectionPosition2 += glm::vec3(mRopeConnectionVelocity * deltaTime, 0.0f, 0.0f);
		}
		if (Inputs::Instance()->IsPressed(SDL_SCANCODE_LEFT))
		{
			mRopeConnectionPosition2 -= glm::vec3(mRopeConnectionVelocity * deltaTime, 0.0f, 0.0f);
		}
	}

	// Update UBOs
	Camera* camera = Engine::Instance()->GetCamera();
	CameraMatricesUBO cameraData{ camera->mProjection, camera->mView, glm::inverse(camera->mView) };
	mCameraUBO->AddData(sizeof(cameraData), &cameraData);

	LightDataUBO lightData{ glm::vec4(mLight->position, 1.0f), glm::vec4(3.2f, 3.2f, 3.2f, 1.0f) };
	mLightUBO->AddData(sizeof(lightData), &lightData);

	Renderer::Instance()->BindUniformBuffer(mCameraUBO, CameraBindingPoint);
	Renderer::Instance()->BindUniformBuffer(mLightUBO, LightBindingPoint);

	// Draw rope nodes (one UBO pair per node, reused from pool)
	int nodeDrawIndex = 0;
	for (int i = 0; i < mRopeNodes.size(); ++i)
	{
		RopeNode& node = mRopeNodes[i];
		glm::mat4 matrix = glm::mat4(1.0f);
		matrix = glm::translate(matrix, node.mPosition);
		matrix = glm::rotate(matrix, glm::radians(node.mRotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
		matrix = glm::rotate(matrix, glm::radians(node.mRotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
		matrix = glm::rotate(matrix, glm::radians(node.mRotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
		matrix = glm::scale(matrix, node.mScale);
		node.mModel = matrix;

		// Ensure we have enough UBO entries in the pool
		while (mNodeUBOs.size() <= nodeDrawIndex)
		{
			NodeDrawUniforms entry;
			entry.mObjectUBO = Renderer::Instance()->CreateUniformBuffer();
			entry.mObjectUBO->Init(sizeof(ObjectMatricesUBO), ObjectBindingPoint);
			entry.mMaterialUBO = Renderer::Instance()->CreateUniformBuffer();
			entry.mMaterialUBO->Init(64, MaterialBindingPoint);
			mNodeUBOs.push_back(entry);
		}

		ObjectMatricesUBO objectData{ glm::inverse(matrix), matrix, matrix };
		mNodeUBOs[nodeDrawIndex].mObjectUBO->AddData(sizeof(objectData), &objectData);

		MaterialUBO mat{};
		mat.mDiffuse = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
		mat.mSpecular = glm::vec4(0.2f, 0.2f, 0.2f, 0.0f);
		mat.mShininess = 1.0f;
		mat.mLighting = 1;
		mat.mUseTexture = 0;
		mNodeUBOs[nodeDrawIndex].mMaterialUBO->AddData(sizeof(mat), &mat);

		Renderer::Instance()->BindUniformBuffer(mNodeUBOs[nodeDrawIndex].mObjectUBO, ObjectBindingPoint);
		Renderer::Instance()->BindUniformBuffer(mNodeUBOs[nodeDrawIndex].mMaterialUBO, MaterialBindingPoint);

		const ShapeData& shape = *node.mRenderData;
		if (shape.mVBO && shape.mIBO)
		{
			Renderer::Instance()->Draw(shape.mVBO, shape.mIBO, Drawing);
		}
		++nodeDrawIndex;
	}
}

void Mechanics::DebugDisplay()
{
	ImGui::Begin("Mechanics");
	ImGui::InputFloat("Rope Friction Constant", &mSpringFriction);
	ImGui::InputFloat("Rope Connection Velocity", &mRopeConnectionVelocity, 0.5f);
	ImGui::InputFloat("Air Friction", &mAirFriction);
	if (ImGui::Button("Reset"))
	{
		ResetNodesAndSprings();
	}
	ImGui::Checkbox("Pause", &mPauseSimulation);
	ImGui::End();
}

void Mechanics::Simulate(float time)
{
	Start();
	Solve();
	Apply(time);
}

void Mechanics::Start()
{
	for (int i = 0; i < mRopeNodes.size(); ++i)
	{
		mRopeNodes[i].mBody.mForce = glm::vec3(0.0f);
	}
}

void Mechanics::Solve()
{
	for (int i = 0; i < mSprings.size(); ++i)
	{
		Spring& spring = mSprings[i];
		glm::vec3 springVec = spring.mMass1->mPosition - spring.mMass2->mPosition;
		float r = glm::length(springVec);

		glm::vec3 force(0.0f);

		if (r != 0.0f)
			force += -mSpringConstant * (springVec / r) * (r - mSpringLength);

		force += -(spring.mMass1->mBody.mVelocity - spring.mMass2->mBody.mVelocity) * mSpringFriction;

		spring.mMass1->mBody.mForce += force;
		spring.mMass2->mBody.mForce += -force;
	}

	for (int i = 1; i < mRopeNodes.size() - 1; ++i)
	{
		Mass& mass = mRopeNodes[i].mBody;
		mass.mForce += mGravitationalForce * mass.mMass;
		mass.mForce += -mAirFriction * mass.mVelocity;
	}
}

void Mechanics::Apply(float time)
{
	float deltaTime = time;

	for (int i = 0; i < mRopeNodes.size(); ++i)
	{
		RopeNode& node = mRopeNodes[i];
		Mass& mass = node.mBody;

		glm::vec3 acceleration = mass.mForce / mass.mMass;
		glm::vec3 kv1 = deltaTime * Func(acceleration, 0.0f, acceleration);
		glm::vec3 kv2 = deltaTime * Func(acceleration, deltaTime / 2.0f, kv1 / 2.0f);
		glm::vec3 kv3 = deltaTime * Func(acceleration, deltaTime / 2.0f, kv2 / 2.0f);
		glm::vec3 kv4 = deltaTime * Func(acceleration, deltaTime, kv3);

		mass.mVelocity += 1.0f / 6.0f * (kv1 + 2.0f * kv2 + 2.0f * kv3 + kv4);

		glm::vec3 kp1 = deltaTime * Func(mass.mVelocity, 0.0f, mass.mVelocity);
		glm::vec3 kp2 = deltaTime * Func(mass.mVelocity, deltaTime / 2.0f, kp1 / 2.0f);
		glm::vec3 kp3 = deltaTime * Func(mass.mVelocity, deltaTime / 2.0f, kp2 / 2.0f);
		glm::vec3 kp4 = deltaTime * Func(mass.mVelocity, deltaTime, kp3);

		node.mPosition += 1.0f / 6.0f * (kp1 + 2.0f * kp2 + 2.0f * kp3 + kp4);
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

void Mechanics::ResetNodesAndSprings()
{
	mRopeNodes.clear();

	RopeNode node;
	node.mPosition = mRopeConnectionPosition1;
	node.mRotation = glm::vec3(0.0f);
	node.mScale = glm::vec3(0.6f, 0.6f, 0.6f);
	node.mModel = glm::mat4(1.0f);
	node.mRenderData = &ShapeManager::Instance()->mShapes[Shapes::SPHERE];
	mRopeNodes.emplace_back(node);

	const float NodeDistance = 0.5f;
	for (int i = 0; i < NodeCount; ++i)
	{
		Mass m{ NodeMass, glm::vec3(0.0f), glm::vec3(0.0f) };
		RopeNode rn;
		rn.mBody = m;
		rn.mPosition = glm::vec3(i * NodeDistance, 4.0f, 0.0f);
		rn.mRotation = glm::vec3(0.0f);
		rn.mScale = glm::vec3(0.3f, 0.3f, 0.3f);
		rn.mModel = glm::mat4(1.0f);
		rn.mRenderData = &ShapeManager::Instance()->mShapes[Shapes::CUBE];
		mRopeNodes.emplace_back(rn);
	}

	node.mPosition = mRopeConnectionPosition2;
	node.mRotation = glm::vec3(0.0f);
	node.mScale = glm::vec3(0.6f, 0.6f, 0.6f);
	node.mModel = glm::mat4(1.0f);
	node.mRenderData = &ShapeManager::Instance()->mShapes[Shapes::SPHERE];
	mRopeNodes.emplace_back(node);

	mSprings.clear();
	for (int i = 0; i < NodeCount + 1; ++i)
	{
		Spring spring{ &mRopeNodes[i], &mRopeNodes[i + 1] };
		mSprings.emplace_back(spring);
	}
}
