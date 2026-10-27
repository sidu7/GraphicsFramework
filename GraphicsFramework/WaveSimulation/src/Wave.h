#pragma once
#include <core/Scene.h>
#include <glm/glm.hpp>
#include <vector>
#include "HeightField.h"

class Light;
class Shader;
class VertexBuffer;
class IndexBuffer;
class UniformBuffer;

class Wave : public Scene
{
public:
	Wave() = default;
	~Wave() override;

	void Init() override;
	void Close() override;
	void Update() override;
	void RenderObject(class Object* object, class Shader* shader) override;
	void DebugDisplay() override;

private:
	// One interleaved vertex: position + normal, stride 24 bytes.
	// The attribute order is the order passed to VertexBuffer::SetDataLayout,
	// which is also the GLSL location order (0 = position, 1 = normal).
	struct WaveVertex
	{
		glm::vec3 mPosition;
		glm::vec3 mNormal;
	};

	Light* mLight = nullptr;
	Shader* mShader = nullptr;

	HeightField mHeightMap;

	VertexBuffer* mVertexBuffer = nullptr;
	IndexBuffer* mIndexBuffer = nullptr;

	UniformBuffer* mObjectUBO = nullptr;    // binding 0
	UniformBuffer* mMaterialUBO = nullptr;  // binding 1
	UniformBuffer* mCameraUBO = nullptr;    // binding 2
	UniformBuffer* mLightUBO = nullptr;     // binding 3

	std::vector<WaveVertex> mVertices;

	glm::vec3 mDiffuseColor{ 0.8f, 0.8f, 1.0f };
	float mWaveIntensity = 0.01f;
};
