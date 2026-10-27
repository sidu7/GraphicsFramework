#include "Wave.h"

#include <cstdint>
#include <iostream>

#include "Core/Camera.h"
#include "Core/Components/Material.h"
#include "Core/Components/Transform.h"
#include "Core/Engine.h"
#include "Core/Inputs.h"
#include "Core/Light.h"
#include "Core/Time.h"
#include "Core/Window.h"
#include "Rendering/IndexBuffer.h"
#include "Rendering/Renderer.h"
#include "Rendering/Shader.h"
#include "Rendering/ShaderRegistry.h"
#include "Rendering/UniformBuffer.h"
#include "Rendering/VertexBuffer.h"

#include "Imgui/imgui.h"

#include <glm/gtc/matrix_transform.hpp>

namespace
{
	// Fixed binding points. 0 and 1 come from res/Settings.json, 2 and 3 are
	// allocated by this scene. See docs section 2.5.
	constexpr uint32_t ObjectBindingPoint = 0;
	constexpr uint32_t MaterialBindingPoint = 1;
	constexpr uint32_t CameraBindingPoint = 2;
	constexpr uint32_t LightBindingPoint = 3;

	// Project-local shader IDs
	enum class WaveShaderID : int32_t
	{
		NONE = 0,
		DRAWING,
	};

	// ObjectMatricesUBO and MaterialDataUBO are the framework's structs
	// (Core/Components/Transform.h, Core/Components/Material.h) — reused so the
	// std140 layout cannot drift away from the GLSL blocks.
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

Wave::~Wave()
{
	Close();
}

void Wave::Close()
{
	delete mLight;
	mLight = nullptr;

	delete mShader;
	mShader = nullptr;

	delete mVertexBuffer;
	mVertexBuffer = nullptr;

	delete mIndexBuffer;
	mIndexBuffer = nullptr;

	delete mObjectUBO;
	mObjectUBO = nullptr;

	delete mMaterialUBO;
	mMaterialUBO = nullptr;

	delete mCameraUBO;
	mCameraUBO = nullptr;

	delete mLightUBO;
	mLightUBO = nullptr;
}

void Wave::Init()
{
	std::cout << "Wave Simulation" << std::endl;

	// The registry must be initialised before any Shader::Init call.
	ShaderRegistry::Instance()->Init("res/shaders/", [](ShaderRegistry* reg) {
		reg->RegisterShader(static_cast<int32_t>(WaveShaderID::DRAWING),
							"Drawing.vert", "Drawing.frag",
							"spirv/Drawing.vert.spv", "spirv/Drawing.frag.spv");
	});

	mLight = new Light(glm::vec3(1.0f, -0.3f, -0.1f));

	Camera* camera = Engine::Instance()->GetCamera();
	camera->mCameraPos = glm::vec3(0.1f, 2.0f, 2.2f);
	camera->pitch = -39.0f;
	camera->yaw = -95.0f;
	camera->mSpeed /= 4.0f;
	camera->CalculateFront();

	mShader = Renderer::Instance()->CreateShader();
	mShader->Init(static_cast<int32_t>(WaveShaderID::DRAWING));

	// --- uniform blocks -------------------------------------------------
	mObjectUBO = Renderer::Instance()->CreateUniformBuffer();
	mObjectUBO->Init(sizeof(ObjectMatricesUBO), ObjectBindingPoint);

	mMaterialUBO = Renderer::Instance()->CreateUniformBuffer();
	mMaterialUBO->Init(sizeof(MaterialDataUBO), MaterialBindingPoint);

	mCameraUBO = Renderer::Instance()->CreateUniformBuffer();
	mCameraUBO->Init(sizeof(CameraMatricesUBO), CameraBindingPoint);

	mLightUBO = Renderer::Instance()->CreateUniformBuffer();
	mLightUBO->Init(sizeof(LightDataUBO), LightBindingPoint);

	// Material never changes, so it is uploaded once.
	MaterialDataUBO materialData{};
	materialData.mDiffuse = glm::vec4(mDiffuseColor, 1.0f);
	materialData.mSpecular = glm::vec4(0.0f);
	materialData.mShininess = 1.0f;
	materialData.mLighting = 1;
	mMaterialUBO->AddData(sizeof(materialData), &materialData);

	// --- geometry -------------------------------------------------------
	mHeightMap.initialize();

	const int width = mHeightMap.grid.width();
	const int height = mHeightMap.grid.height();
	const int vertexCount = width * height;

	mVertices.resize(static_cast<size_t>(vertexCount));

	mVertexBuffer = Renderer::Instance()->CreateVertexBuffer();
	mVertexBuffer->SetDataLayout({ VertexFormat::Vec3, VertexFormat::Vec3 });
	// Allocates storage on both backends; contents are written every frame.
	mVertexBuffer->AddDynamicData(nullptr, static_cast<unsigned int>(vertexCount * sizeof(WaveVertex)));

	// Two triangles per grid cell, uint16 indices (DeviceGL::Draw hardcodes
	// GL_UNSIGNED_SHORT, so 32-bit indices are not an option).
	std::vector<uint16_t> indices;
	indices.reserve(static_cast<size_t>(6) * (width - 1) * (height - 1));

	for (int j = 1; j < height; ++j)
	{
		for (int i = 1; i < width; ++i)
		{
			const uint16_t v00 = static_cast<uint16_t>(width * (j - 1) + (i - 1));
			const uint16_t v10 = static_cast<uint16_t>(width * (j - 1) + i);
			const uint16_t v11 = static_cast<uint16_t>(width * j + i);
			const uint16_t v01 = static_cast<uint16_t>(width * j + (i - 1));

			indices.push_back(v00);
			indices.push_back(v10);
			indices.push_back(v11);

			indices.push_back(v00);
			indices.push_back(v11);
			indices.push_back(v01);
		}
	}

	mIndexBuffer = Renderer::Instance()->CreateIndexBuffer();
	mIndexBuffer->AddData(indices.data(), static_cast<unsigned int>(indices.size()), IndexType::UInt16);

	// --- state ----------------------------------------------------------
	Renderer::Instance()->SetClearColor(glm::vec3(1.0f, 1.0f, 1.0f));
	Renderer::Instance()->SetDepthTest(true);
	// The grid is a single-sided sheet; culling it would make the picture depend
	// on the winding order, which differs between the two backends' conventions.
	Renderer::Instance()->SetCullingFace(CullFace::None);

	mWaveIntensity = 0.01f;
}

void Wave::Update()
{
	Camera* camera = Engine::Instance()->GetCamera();

	CameraMatricesUBO cameraData{ camera->mProjection, camera->mView, glm::inverse(camera->mView) };
	mCameraUBO->AddData(sizeof(cameraData), &cameraData);

	LightDataUBO lightData{ glm::vec4(mLight->position, 1.0f), glm::vec4(1.0f) };
	mLightUBO->AddData(sizeof(lightData), &lightData);

	// The grid is built in (i, j, height) space and mapped onto the XZ plane.
	const glm::mat4 I = glm::mat4(1.0f);
	const glm::mat4 modelTrans =
		glm::rotate(I, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f)) *
		glm::translate(I, glm::vec3(-1.0f, -1.0f, 0.0f)) *
		glm::scale(I, glm::vec3(2.0f / mHeightMap.grid.width(),
								2.0f / mHeightMap.grid.height(),
								1.0f));

	// normaltr must be the inverse of the model matrix (the old shader used
	// exactly this), not the transposed inverse, to keep the shading identical.
	ObjectMatricesUBO objectData{ glm::inverse(modelTrans), modelTrans, modelTrans };
	mObjectUBO->AddData(sizeof(objectData), &objectData);

	Renderer::Instance()->BindUniformBuffer(mObjectUBO, ObjectBindingPoint);
	Renderer::Instance()->BindUniformBuffer(mMaterialUBO, MaterialBindingPoint);
	Renderer::Instance()->BindUniformBuffer(mCameraUBO, CameraBindingPoint);
	Renderer::Instance()->BindUniformBuffer(mLightUBO, LightBindingPoint);

	// Rebuild positions and normals on the CPU, then upload the whole
	// interleaved buffer in one call.
	const int width = mHeightMap.grid.width();
	const int height = mHeightMap.grid.height();

	for (int j = 0; j < height; ++j)
	{
		for (int i = 0; i < width; ++i)
		{
			const int index = width * j + i;

			const float h = glm::clamp(mHeightMap.grid(i, j).real(), 0.0f,
									   mHeightMap.max_height * mWaveIntensity);

			mVertices[index].mPosition = glm::vec3(static_cast<float>(i),
												   static_cast<float>(j), h);

			float dx = 0.0f;
			float dy = 0.0f;

			if (i > 0 && i + 1 < width)
			{
				dx = 0.5f * (mHeightMap.grid(i + 1, j).real() - mHeightMap.grid(i - 1, j).real());
			}
			else if (i > 0)
			{
				dx = mHeightMap.grid(i, j).real() - mHeightMap.grid(i - 1, j).real();
			}
			else
			{
				dx = mHeightMap.grid(i + 1, j).real() - mHeightMap.grid(i, j).real();
			}

			if (j > 0 && j + 1 < height)
			{
				dy = 0.5f * (mHeightMap.grid(i, j + 1).real() - mHeightMap.grid(i, j - 1).real());
			}
			else if (j > 0)
			{
				dy = mHeightMap.grid(i, j).real() - mHeightMap.grid(i, j - 1).real();
			}
			else
			{
				dy = mHeightMap.grid(i, j + 1).real() - mHeightMap.grid(i, j).real();
			}

			mVertices[index].mNormal = glm::vec3(-dx, -dy, 1.0f);
		}
	}

	mVertexBuffer->AddSubData(mVertices.data(),
							  static_cast<unsigned int>(mVertices.size() * sizeof(WaveVertex)),
							  0);

	Renderer::Instance()->Draw(mVertexBuffer, mIndexBuffer, mShader);

	if (Inputs::Instance()->IsMouseClicked(SDL_BUTTON_LEFT))
	{
		glm::vec2 mousePos = Inputs::Instance()->GetMouseLocation();
		mousePos /= Window::Instance()->GetWindowSize();
		mHeightMap.handle_input(mousePos);
	}

	mHeightMap.update(Time::Instance()->deltaTime);
}

void Wave::RenderObject(Object* object, Shader* shader)
{
	// This scene draws directly; ObjectManager objects are not used.
}

void Wave::DebugDisplay()
{
	if (ImGui::Button("Reset"))
	{
		mHeightMap.initialize();
	}
	ImGui::SliderFloat("Wave Intensity", &mWaveIntensity, 0.01f, 1.f);
}
