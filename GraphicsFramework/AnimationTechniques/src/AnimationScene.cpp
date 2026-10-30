#include "AnimationScene.h"

#include "Core/Engine.h"
#include "Core/Camera.h"
#include "Core/Light.h"
#include "Core/ImguiManager.h"
#include "Core/ShapeManager.h"
#include "Core/Time.h"
#include "Assets/Model.h"
#include "Rendering/Shader.h"
#include "Rendering/Renderer.h"
#include "Rendering/ShaderRegistry.h"
#include "Rendering/UniformBuffer.h"
#include "Rendering/VertexBuffer.h"
#include "Rendering/IndexBuffer.h"
#include "Core/Components/Transform.h"
#include "Core/Components/Shape.h"
#include "Core/Components/Material.h"

#include "Imgui/imgui.h"

#include <glm/gtc/matrix_transform.hpp>

#include <fstream>
#include <sstream>

namespace
{
	// Fixed binding points. 0 and 1 come from Settings.json, 2-4 are allocated by this scene.
	constexpr uint32_t ObjectBindingPoint = 0;
	constexpr uint32_t MaterialBindingPoint = 1;
	constexpr uint32_t CameraBindingPoint = 2;
	constexpr uint32_t LightBindingPoint = 3;
	constexpr uint32_t BoneBindingPoint = 4;

	// App-local material block. 64 bytes, 16-byte aligned. Must match res/Shaders/*.frag.
	struct AppMaterialUBO
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

	// Framework structs (reused from Core/Components/Transform.h and Material.h)
	// but declared here so the C++ layout matches the GLSL std140 block.
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
}

// ------------------------------------------------------------------- helpers ----

AnimationScene::~AnimationScene()
{
	Close();
}

AnimationScene::DrawUniforms& AnimationScene::NextDrawUniforms()
{
	if (mDrawIndex >= mDrawUniforms.size())
	{
		DrawUniforms entry;
		entry.mObjectUBO = Renderer::Instance()->CreateUniformBuffer();
		entry.mObjectUBO->Init(sizeof(ObjectMatricesUBO), ObjectBindingPoint);

		entry.mMaterialUBO = Renderer::Instance()->CreateUniformBuffer();
		entry.mMaterialUBO->Init(sizeof(AnimationMaterialUBO), MaterialBindingPoint);

		mDrawUniforms.push_back(entry);
	}

	return mDrawUniforms[mDrawIndex++];
}

void AnimationScene::UploadBones()
{
	const size_t boneCount = std::min(demoModel->mBones.size(), static_cast<size_t>(128));

	for (size_t i = 0; i < boneCount; ++i)
	{
		Bone& bone = demoModel->mBones[i];
		mBonesTransformation[i] = bone.mCurrentGlobalTransformation * bone.mOffset;
	}

	for (size_t i = boneCount; i < 128; ++i)
	{
		mBonesTransformation[i] = glm::mat4(1.0f);
	}

	mBoneUBO->AddData(sizeof(glm::mat4) * 128, mBonesTransformation.data());
}

void AnimationScene::BuildCurveBuffers()
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
	mCurvePointsSize = static_cast<unsigned int>(curvePoints.size());

	if (mCurveVBO == nullptr)
	{
		mCurveVBO = Renderer::Instance()->CreateVertexBuffer();
		mCurveVBO->SetDataLayout({ VertexFormat::Vec3, VertexFormat::Vec3 });
		mCurveIBO = Renderer::Instance()->CreateIndexBuffer();
	}

	// Interleaved vertex: position (vec3 with w = curve parameter) + dummy normal (vec3)
	struct CurveVertex
	{
		glm::vec3 mPosition;
		glm::vec3 mNormal;
	};

	std::vector<CurveVertex> vertices;
	vertices.reserve(curvePoints.size());
	for (const glm::vec4& cp : curvePoints)
	{
		vertices.push_back({ glm::vec3(cp.x, cp.y, cp.z), glm::vec3(0.0f, 0.0f, 1.0f) });
	}

	mCurveVBO->AddDynamicData(vertices.data(),
		static_cast<unsigned int>(vertices.size() * sizeof(CurveVertex)));

	// Build uint16 index pairs: each consecutive pair is a line segment
	std::vector<uint16_t> indices;
	indices.reserve(mCurvePointsSize);
	for (unsigned int i = 0; i < mCurvePointsSize; ++i)
	{
		indices.push_back(i);
		indices.push_back(i + 1);
	}
	// The last segment is drawn twice (loop back), skip the last index pair
	indices.pop_back();
	indices.pop_back();

	mCurveIBO->AddData(indices.data(), static_cast<unsigned int>(indices.size()), IndexType::UInt16);
}

void AnimationScene::BuildBonesBuffers()
{
	// Called once in Init() to create the bone line VBO; contents updated in Update().
}

// ------------------------------------------------------------------- Init / Close ----

void AnimationScene::Init()
{
	mMode = Mode::Animation;

	// Initialize shader registry
	ShaderRegistry::Instance()->Init("res/Shaders/", [](ShaderRegistry* reg) {
		reg->RegisterShader(static_cast<int32_t>(AnimationShaderID::MODEL_DRAW),
							"ModelDraw.vert", "ModelDraw.frag",
							"spirv/ModelDraw.vert.spv", "spirv/ModelDraw.frag.spv");
		reg->RegisterShader(static_cast<int32_t>(AnimationShaderID::DRAWING),
							"Drawing.vert", "Drawing.frag",
							"spirv/Drawing.vert.spv", "spirv/Drawing.frag.spv");
	});

	demoModel = new Model();
	demoModel->LoadModel("res/ArcherWalking.fbx");
	demoModel->AddAnimation("res/ArcherDancing.fbx");
	demoModel->AddAnimation("res/ArcherShooting.fbx");
	demoModel->AddAnimation("res/PickUp.fbx");

	/*
	 *IK bones
	 *RightArm
	 *RightForeArm
	 *RightHand
	 */
	for (int i = static_cast<int>(demoModel->mBones.size()) - 1; i >= 0; --i)
	{
		if (demoModel->mBones[i].mName == "RightArm" ||
			demoModel->mBones[i].mName == "RightForeArm" ||
			demoModel->mBones[i].mName == "RightHand")
		{
			mIKBones.push_back(static_cast<unsigned int>(i));
		}
		demoModel->mBones[i].mIKTransformation = glm::mat4(1.0f);
	}
	mEndEffector = glm::vec3(0.0f);
	mDoOnce = true;
	IKDuration = 1.0f;

	animation = demoModel->mAnimations[0];

	Engine::Instance()->GetCamera()->pitch = -8.0f;
	Engine::Instance()->GetCamera()->yaw = 4.8f;
	Engine::Instance()->GetCamera()->mCameraPos = glm::vec3(-126.0f, 26.0f, -14.0f);
	Engine::Instance()->GetCamera()->CalculateFront();

	mBonesTransformation.resize(demoModel->mBones.size());

	light = new Light(glm::vec3(10.0f, 100.0f, 40.0f));

	modelDrawShader = Renderer::Instance()->CreateShader();
	modelDrawShader->Init(static_cast<int32_t>(AnimationShaderID::MODEL_DRAW));
	drawingShader = Renderer::Instance()->CreateShader();
	drawingShader->Init(static_cast<int32_t>(AnimationShaderID::DRAWING));

	// --- uniform blocks -------------------------------------------------
	mCameraUBO = Renderer::Instance()->CreateUniformBuffer();
	mCameraUBO->Init(sizeof(CameraMatricesUBO), CameraBindingPoint);

	mLightUBO = Renderer::Instance()->CreateUniformBuffer();
	mLightUBO->Init(sizeof(LightDataUBO), LightBindingPoint);

	mBoneUBO = Renderer::Instance()->CreateUniformBuffer();
	mBoneUBO->Init(sizeof(glm::mat4) * 128, BoneBindingPoint);

	mBonesTransformation.assign(128, glm::mat4(1.0f));

	// --- curve ----------------------------------------------------------
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
	BuildCurveBuffers();

	mPathMatrix = glm::mat4(1.0f);
	showControlWindow = false;
	mGoalPosition = glm::vec3(-5.0f, 14.0f, 10.0f);
	maxIterations = 10;
	sqrDistanceError = 0.02f;

	// --- state ----------------------------------------------------------
	Renderer::Instance()->SetClearColor(glm::vec3(0.2f, 0.2f, 0.2f));
	Renderer::Instance()->SetDepthTest(true);
	Renderer::Instance()->SetCullingFace(CullFace::Front);
}

void AnimationScene::Close()
{
	delete demoModel;
	demoModel = nullptr;

	delete light;
	light = nullptr;

	delete modelDrawShader;
	modelDrawShader = nullptr;

	delete drawingShader;
	drawingShader = nullptr;

	delete mCurveVBO;
	mCurveVBO = nullptr;
	delete mCurveIBO;
	mCurveIBO = nullptr;

	delete mCameraUBO;
	mCameraUBO = nullptr;

	delete mLightUBO;
	mLightUBO = nullptr;

	delete mBoneUBO;
	mBoneUBO = nullptr;

	for (DrawUniforms& du : mDrawUniforms)
	{
		delete du.mObjectUBO;
		delete du.mMaterialUBO;
	}
	mDrawUniforms.clear();

	controlPoints.clear();
	controlPointsMatrices.clear();
	mArcLengthTable.clear();
}

// ------------------------------------------------------------------- RenderObject ----

void AnimationScene::RenderObject(Object* object, Shader* shader)
{
	// This scene draws directly; ObjectManager objects are not used.
}

// ------------------------------------------------------------------- Update ----

void AnimationScene::Update()
{
	if (!isPaused)
	{
		PathRunTime += Time::Instance()->deltaTime;
	}

	// Camera + light UBOs
	Camera* camera = Engine::Instance()->GetCamera();
	CameraMatricesUBO cameraData{ camera->mProjection, camera->mView, glm::inverse(camera->mView) };
	mCameraUBO->AddData(sizeof(cameraData), &cameraData);

	LightDataUBO lightData{ glm::vec4(light->position, 1.0f), glm::vec4(3.2f, 3.2f, 3.2f, 1.0f) };
	mLightUBO->AddData(sizeof(lightData), &lightData);

	// Shared UBO bindings (same for all draws)
	Renderer::Instance()->BindUniformBuffer(mCameraUBO, CameraBindingPoint);
	Renderer::Instance()->BindUniformBuffer(mLightUBO, LightBindingPoint);
	Renderer::Instance()->BindUniformBuffer(mBoneUBO, BoneBindingPoint);

	// Animation time update
	float speed = mSpeed;
	if (mPathWalk)
	{
		float distance;
		if (PathRunTime < t1)
		{
			speed = mSpeed * PathRunTime / t1;
			distance = 0.5f * mSpeed / t1 * PathRunTime * PathRunTime;
		}
		else if (PathRunTime > t1 && PathRunTime <= t2)
		{
			speed = mSpeed;
			distance = mSpeed * t1 * 0.5f + mSpeed * (PathRunTime - t1);
		}
		else if (PathRunTime > t2 && PathRunTime <= totalRunTime)
		{
			speed = mSpeed * (totalRunTime - PathRunTime) / (totalRunTime - t2);
			distance = mSpeed * t1 * 0.5f + mSpeed * (t2 - t1) +
				(mSpeed - (mSpeed * (PathRunTime - t2) / (totalRunTime - t2)) * 0.5f) *
				(PathRunTime - t2);
		}
		else
		{
			PathRunTime = 0.0f;
			distance = 0.0f;
			speed = 0.0f;
		}

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
	else
	{
		mPathMatrix = glm::mat4(1.0f);
	}

	if (!isPaused)
	{
		float speedFactor = speed / mSpeed;
		AnimationRunTime += Time::Instance()->deltaTime * speedFactor;
	}

	glm::mat4 matrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));
	matrix = glm::scale(matrix, glm::vec3(0.1f));

	if (mMode == Mode::Animation)
	{
		AnimatorUpdate(animation);
	}
	if (mMode == Mode::IK)
	{
		if (mDoOnce)
		{
			AnimatorUpdate(animation);
			mDoOnce = false;
		}
		IKUpdate(matrix, AnimationRunTime / IKDuration);

		if (mEndEffector == glm::vec3(0.0f))
		{
			mEndEffector = demoModel->mBones[mIKBones[0]].mCurrentGlobalTransformation[3];
		}
	}

	// Upload bone matrices
	for (unsigned int i = 0; i < demoModel->mBones.size(); ++i)
	{
		Bone& bone = demoModel->mBones[i];
		mBonesTransformation[i] = bone.mCurrentGlobalTransformation * bone.mOffset;
	}
	// Pad remaining bones with identity
	for (size_t i = demoModel->mBones.size(); i < 128; ++i)
	{
		mBonesTransformation[i] = glm::mat4(1.0f);
	}
	mBoneUBO->AddData(sizeof(glm::mat4) * 128, mBonesTransformation.data());

	mDrawIndex = 0;

	// --- skinned model --------------------------------------------------
	if (drawModel)
	{
		DrawUniforms& uniforms = NextDrawUniforms();

		const glm::mat4 model = matrix;
		ObjectMatricesUBO objectData{ glm::inverse(mPathMatrix * model), mPathMatrix * model, mPathMatrix * model };
		uniforms.mObjectUBO->AddData(sizeof(objectData), &objectData);

		AnimationMaterialUBO material{};
		material.mDiffuse = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
		material.mSpecular = glm::vec4(1.0f, 1.0f, 1.0f, 0.0f);
		material.mShininess = 1.0f;
		material.mLighting = 1;
		material.mUseTexture = 0;
		uniforms.mMaterialUBO->AddData(sizeof(material), &material);

		Renderer::Instance()->BindUniformBuffer(uniforms.mObjectUBO, ObjectBindingPoint);
		Renderer::Instance()->BindUniformBuffer(uniforms.mMaterialUBO, MaterialBindingPoint);

		demoModel->Draw(modelDrawShader);
	}

	// --- bones debug ----------------------------------------------------
	if (drawBones)
	{
		if (drawModel)
		{
			Renderer::Instance()->SetDepthTest(false);
		}

		// Draw bone cubes
		DrawUniforms& cubeUniforms = NextDrawUniforms();
		for (unsigned int i = 0; i < demoModel->mBones.size(); ++i)
		{
			cubeUniforms.mObjectUBO->AddData(sizeof(ObjectMatricesUBO),
				&ObjectMatricesUBO{ glm::inverse(demoModel->mBones[i].mCurrentGlobalTransformation),
					demoModel->mBones[i].mCurrentGlobalTransformation,
					demoModel->mBones[i].mCurrentGlobalTransformation });

			AppMaterialUBO mat{};
			mat.mDiffuse = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
			mat.mLighting = 0;
			mat.mUseTexture = 0;
			cubeUniforms.mMaterialUBO->AddData(sizeof(mat), &mat);

			Renderer::Instance()->BindUniformBuffer(cubeUniforms.mObjectUBO, ObjectBindingPoint);
			Renderer::Instance()->BindUniformBuffer(cubeUniforms.mMaterialUBO, MaterialBindingPoint);

			const ShapeData& shape = ShapeManager::Instance()->mShapes[Shapes::CUBE];
			if (shape.mVBO && shape.mIBO)
			{
				Renderer::Instance()->Draw(shape.mVBO, shape.mIBO, drawingShader);
			}
		}

		// Draw bone lines
		{
			DrawUniforms& lineUniforms = NextDrawUniforms();

			lineUniforms.mObjectUBO->AddData(sizeof(ObjectMatricesUBO),
				&ObjectMatricesUBO{ glm::mat4(1.0f), glm::mat4(1.0f), glm::mat4(1.0f) });

			AppMaterialUBO mat{};
			mat.mDiffuse = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
			mat.mLighting = 0;
			mat.mUseTexture = 0;
			lineUniforms.mMaterialUBO->AddData(sizeof(mat), &mat);

			Renderer::Instance()->BindUniformBuffer(lineUniforms.mObjectUBO, ObjectBindingPoint);
			Renderer::Instance()->BindUniformBuffer(lineUniforms.mMaterialUBO, MaterialBindingPoint);

			// Build bone lines
			struct BoneVertex
			{
				glm::vec3 mPosition;
				glm::vec3 mNormal;
			};

			std::vector<BoneVertex> vertices;
			for (unsigned int i = 1; i < demoModel->mBones.size(); ++i)
			{
				glm::mat4 trans = demoModel->mBones[i].mCurrentGlobalTransformation;
				vertices.push_back({ (glm::vec3)trans[3], glm::vec3(0.0f, 0.0f, 1.0f) });

				glm::mat4 parentTrans = demoModel->mBones[demoModel->mBones[i].mParentIndex].mCurrentGlobalTransformation;
				vertices.push_back({ (glm::vec3)parentTrans[3], glm::vec3(0.0f, 0.0f, 1.0f) });
			}

			VertexBuffer* bonesVBO = Renderer::Instance()->CreateVertexBuffer();
			bonesVBO->SetDataLayout({ VertexFormat::Vec3, VertexFormat::Vec3 });
			bonesVBO->AddData(vertices.data(), static_cast<unsigned int>(vertices.size() * sizeof(BoneVertex)));

			Renderer::Instance()->DebugDrawLines(bonesVBO, nullptr, drawingShader);
			delete bonesVBO;
		}

		Renderer::Instance()->SetDepthTest(true);
	}

	// --- control points -------------------------------------------------
	DrawUniforms& cpUniforms = NextDrawUniforms();
	for (unsigned int i = 0; i < controlPoints.size(); ++i)
	{
		glm::mat4 controlModel = glm::translate(glm::mat4(1.0f), glm::vec3(controlPoints[i]));
		controlModel = glm::translate(controlModel, glm::vec3(0.0f, 0.125f, 0.0f));
		controlModel = glm::scale(controlModel, glm::vec3(0.25f));

		ObjectMatricesUBO objectData{ glm::inverse(controlModel), controlModel, controlModel };
		cpUniforms.mObjectUBO->AddData(sizeof(objectData), &objectData);

		AppMaterialUBO mat{};
		mat.mDiffuse = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);
		mat.mLighting = 1;
		mat.mUseTexture = 0;
		cpUniforms.mMaterialUBO->AddData(sizeof(mat), &mat);

		Renderer::Instance()->BindUniformBuffer(cpUniforms.mObjectUBO, ObjectBindingPoint);
		Renderer::Instance()->BindUniformBuffer(cpUniforms.mMaterialUBO, MaterialBindingPoint);

		const ShapeData& shape = ShapeManager::Instance()->mShapes[Shapes::CUBE];
		if (shape.mVBO && shape.mIBO)
		{
			Renderer::Instance()->Draw(shape.mVBO, shape.mIBO, drawingShader);
		}
	}

	// --- curve polyline -----------------------------------------------
	{
		DrawUniforms& curveUniforms = NextDrawUniforms();

		const glm::mat4 model(1.0f);
		ObjectMatricesUBO objectData{ model, model, model };
		curveUniforms.mObjectUBO->AddData(sizeof(objectData), &objectData);

		AppMaterialUBO mat{};
		mat.mDiffuse = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);
		mat.mLighting = 0;
		mat.mUseTexture = 0;
		curveUniforms.mMaterialUBO->AddData(sizeof(mat), &mat);

		Renderer::Instance()->BindUniformBuffer(curveUniforms.mObjectUBO, ObjectBindingPoint);
		Renderer::Instance()->BindUniformBuffer(curveUniforms.mMaterialUBO, MaterialBindingPoint);

		if (mCurveVBO && mCurveIBO)
		{
			Renderer::Instance()->DebugDrawLines(mCurveVBO, mCurveIBO, drawingShader);
		}
	}

	// --- floor ----------------------------------------------------------
	{
		DrawUniforms& floorUniforms = NextDrawUniforms();

		glm::mat4 floorModel = glm::mat4(1.0f);
		floorModel = glm::rotate(floorModel, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		floorModel = glm::scale(floorModel, glm::vec3(500.0f));

		ObjectMatricesUBO objectData{ glm::inverse(floorModel), floorModel, floorModel };
		floorUniforms.mObjectUBO->AddData(sizeof(objectData), &objectData);

		AppMaterialUBO mat{};
		mat.mDiffuse = glm::vec4(0.0f, 0.5f, 0.0f, 1.0f);
		mat.mSpecular = glm::vec4(0.2f, 0.2f, 0.2f, 0.0f);
		mat.mShininess = 1.0f;
		mat.mLighting = 1;
		mat.mUseTexture = 0;
		floorUniforms.mMaterialUBO->AddData(sizeof(mat), &mat);

		Renderer::Instance()->BindUniformBuffer(floorUniforms.mObjectUBO, ObjectBindingPoint);
		Renderer::Instance()->BindUniformBuffer(floorUniforms.mMaterialUBO, MaterialBindingPoint);

		const ShapeData& shapeQuad = ShapeManager::Instance()->mShapes[Shapes::QUAD];
		if (shapeQuad.mVBO && shapeQuad.mIBO)
		{
			Renderer::Instance()->Draw(shapeQuad.mVBO, shapeQuad.mIBO, drawingShader);
		}
	}

	// --- goal sphere ----------------------------------------------------
	{
		DrawUniforms& goalUniforms = NextDrawUniforms();

		glm::mat4 goalModel = glm::translate(glm::mat4(1.0f), mGoalPosition);
		goalModel = glm::scale(goalModel, glm::vec3(1.0f));

		ObjectMatricesUBO objectData{ glm::inverse(goalModel), goalModel, goalModel };
		goalUniforms.mObjectUBO->AddData(sizeof(objectData), &objectData);

		AppMaterialUBO mat{};
		mat.mDiffuse = glm::vec4(0.5f, 0.0f, 0.0f, 1.0f);
		mat.mSpecular = glm::vec4(0.2f, 0.2f, 0.2f, 0.0f);
		mat.mShininess = 1.0f;
		mat.mLighting = 1;
		mat.mUseTexture = 0;
		goalUniforms.mMaterialUBO->AddData(sizeof(mat), &mat);

		Renderer::Instance()->BindUniformBuffer(goalUniforms.mObjectUBO, ObjectBindingPoint);
		Renderer::Instance()->BindUniformBuffer(goalUniforms.mMaterialUBO, MaterialBindingPoint);

		const ShapeData& shapeSphere = ShapeManager::Instance()->mShapes[Shapes::SPHERE];
		if (shapeSphere.mVBO && shapeSphere.mIBO)
		{
			Renderer::Instance()->Draw(shapeSphere.mVBO, shapeSphere.mIBO, drawingShader);
		}
	}
}

// ------------------------------------------------------------------- DebugDisplay ----

void AnimationScene::DebugDisplay()
{
	ImGui::Begin("Animation");

	const std::vector<std::string> Modes = {
		"Animation",
		"Inverse Kinematics"
	};
	if (ImGui::BeginCombo("Project Mode", Modes[static_cast<uint16_t>(mMode)].c_str()))
	{
		for (uint16_t i = 0; i < Modes.size(); ++i)
		{
			if (ImGui::Selectable(Modes[i].c_str(), i == static_cast<uint16_t>(mMode)))
			{
				mMode = static_cast<Mode>(i);
				switch (mMode)
				{
					case Mode::Animation:
						AnimationRunTime = 0.01f;
						break;

					case Mode::IK:
						mDoOnce = true;
						animation = demoModel->mAnimations[3]; // Pickup animation
						break;
				}
			}
		}
		ImGui::EndCombo();
	}
	if (ImGui::BeginCombo("Select Animation", animation.c_str()))
	{
		for (unsigned int i = 0; i < demoModel->mAnimations.size(); ++i)
		{
			if (ImGui::Selectable(demoModel->mAnimations[i].c_str(), demoModel->mAnimations[i] == animation))
			{
				animation = demoModel->mAnimations[i];
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
		AnimationRunTime -= 2.0f * Time::Instance()->deltaTime;
	}
	ImGui::SameLine();
	if (ImGui::Button("Step -"))
	{
		isPaused = true;
		AnimationRunTime -= Time::Instance()->deltaTime;
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
		AnimationRunTime += Time::Instance()->deltaTime;
	}
	ImGui::SameLine();
	if (ImGui::Button("Step ++"))
	{
		isPaused = true;
		AnimationRunTime += 2.0f * Time::Instance()->deltaTime;
	}
	if (ImGui::Checkbox("Walk on Path", &mPathWalk))
	{
		PathRunTime = 0.0f;
		if (mPathWalk)
		{
			animation = demoModel->mAnimations[0]; // Walking Animation
		}
	}
	if (ImGui::Button(showControlWindow ? "Hide Control Points Window" : "Show Control Points Window"))
	{
		showControlWindow = !showControlWindow;
	}
	if (ImGui::InputFloat("ArcTable Tolerance", &mTolerance, 0.0005f, 0.005f, "%.4f"))
	{
		mTolerance = mTolerance < 0.0f ? 0.0001f : mTolerance;
		CreateAxisLengthTable();
	}
	ImGui::InputFloat("IK Duration", &IKDuration, 0.5f);
	ImGui::Text("GoalPosition");
	if (ImGui::InputFloat("X", &mGoalPosition.x, 0.2f) ||
		ImGui::InputFloat("Y", &mGoalPosition.y, 0.2f) ||
		ImGui::InputFloat("Z", &mGoalPosition.z, 0.2f))
	{
		AnimationRunTime = 0.0f;
		mEndEffector = demoModel->mBones[mIKBones[0]].mCurrentGlobalTransformation[3];
	}

	if (showControlWindow)
	{
		ImGui::Begin("Control Points");
		if (ImGui::Button("New Control Point"))
		{
			controlPoints.emplace_back(controlPoints[controlPoints.size() - 1]);
			RecalculateMatrices();
			CreateAxisLengthTable();
			BuildCurveBuffers();
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
				BuildCurveBuffers();
			}
		}
		ImGui::End();
	}
	ImGui::End();
}

// ------------------------------------------------------------------- animation math (unchanged) ----

void AnimationScene::AnimatorUpdate(std::string animName)
{
	mBonesTransformation.clear();

	for (unsigned int i = 0; i < demoModel->mBones.size(); ++i)
	{
		Bone& bone = demoModel->mBones[i];
		glm::mat4 trans = glm::mat4(1.0f);
		if (bone.isAnimated)
		{
			//interpolate for timeframe
			AnimationData& anim = bone.mAnimations[animName];
			double TimeinTicks = AnimationRunTime * anim.mTicksPerSec;
			double timeFrame = glm::mod(TimeinTicks, anim.mDuration);
			if (mMode == Mode::IK)
			{
				timeFrame = anim.mDuration - 2; // Static IK pose
			}
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
			trans = translate * rot * scaling;
		}
		else
		{
			trans = bone.mTransformation;
		}
		unsigned int parent = bone.mParentIndex;
		bone.mCurrentLocalTransformation = trans;
		if (parent != -1)
		{
			bone.mCurrentGlobalTransformation = demoModel->mBones[parent].mCurrentGlobalTransformation * trans;
		}
		else
		{
			bone.mCurrentGlobalTransformation = trans;
		}

		mBonesTransformation.push_back(bone.mCurrentGlobalTransformation);
	}
}

glm::vec4 AnimationScene::GetPointOnCurve(float t, glm::mat4& controlPointMatrix)
{
	return glm::vec4(t * t * t, t * t, t, 1.0f) * CurveMatrix * controlPointMatrix;
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

void AnimationScene::IKUpdate(glm::mat4 model, float time)
{
	glm::vec3 Goal = glm::inverse(model) * glm::vec4(mGoalPosition, 1.0);
	glm::vec3 currentEndEffector = (model * demoModel->mBones[mIKBones[0]].mCurrentGlobalTransformation)[3];
	if (time > 1.0f) time = 1.0f;
	glm::vec3 Gprime = (1 - time) * mEndEffector + (time)*Goal;
	//Gprime = Goal;
	float sqrDistance = glm::length(currentEndEffector - mGoalPosition);

	int iterationCount = 0;
	while (sqrDistance > sqrDistanceError && iterationCount <= maxIterations)
	{
		for (int j = 1; j < static_cast<int>(mIKBones.size()); ++j)
		{
			glm::vec3 originOfRotation = demoModel->mBones[mIKBones[j]].mCurrentGlobalTransformation[3];
			currentEndEffector = demoModel->mBones[mIKBones[0]].mCurrentGlobalTransformation[3];
			glm::vec3 B = glm::normalize(currentEndEffector - originOfRotation);
			glm::vec3 C = glm::normalize(Gprime - originOfRotation);
			float dot = glm::dot(B, C);
			if (dot >= 0.9999f)
			{
				continue;
			}
			float alpha = glm::acos(dot);
			if (j == 1)
			{
				glm::vec3 shoulder = demoModel->mBones[mIKBones[j + 1]].mCurrentGlobalTransformation[3];
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
			axis = glm::inverse(demoModel->mBones[mIKBones[j]].mCurrentGlobalTransformation) * glm::vec4(axis, 0.0f);
			float alen = glm::length(axis);
			glm::quat q = glm::quat(cos(alpha / 2.0f), sin(alpha / 2.0f) * axis.x / alen,
				sin(alpha / 2.0f) * axis.y / alen, sin(alpha / 2.0f) * axis.z / alen);
			glm::normalize(q);
			demoModel->mBones[mIKBones[j]].mIKTransformation *= glm::toMat4(q);

			for (int k = j; k >= 0; --k)
			{
				Bone& bone = demoModel->mBones[mIKBones[k]];
				bone.mCurrentGlobalTransformation = demoModel->mBones[bone.mParentIndex].mCurrentGlobalTransformation *
					bone.mCurrentLocalTransformation * bone.mIKTransformation;
			}

			currentEndEffector = (model * demoModel->mBones[mIKBones[0]].mCurrentGlobalTransformation)[3];

			sqrDistance = glm::length(currentEndEffector - mGoalPosition);

			if (sqrDistance <= sqrDistanceError)
			{
				goto Update_children;
			}
		}

		currentEndEffector = (model * demoModel->mBones[mIKBones[0]].mCurrentGlobalTransformation)[3];
		sqrDistance = glm::length(currentEndEffector - mGoalPosition);
		++iterationCount;
	}

Update_children:
	//Update end effectors children
	for (unsigned int i = 0; i < demoModel->mBones.size(); ++i)
	{
		if (std::find(mIKBones.begin(), mIKBones.end(), i) != mIKBones.end())
		{
			continue;
		}
		Bone& bone = demoModel->mBones[i];
		if (bone.mParentIndex != -1)
		{
			bone.mCurrentGlobalTransformation = demoModel->mBones[bone.mParentIndex].mCurrentGlobalTransformation *
				bone.mCurrentLocalTransformation;
		}
	}
}

void AnimationScene::RecalculateMatrices()
{
	controlPointsMatrices.clear();

	for (unsigned int i = 0; i + 3 < controlPoints.size(); ++i)
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
		adaptiveAlgoStack.push(MAKE_TUPLE(0.0f, 1.0f, i));

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
				mArcLengthTable.emplace_back(MAKE_TUPLE(distance + A + B, sb, index));
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

inline std::pair<float, int> AnimationScene::SearchInTable(float distance)
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
