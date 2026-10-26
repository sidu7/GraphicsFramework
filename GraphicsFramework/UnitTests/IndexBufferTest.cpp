#include "IndexBufferTest.h"

#include "Core/Core.h"
#include "Rendering/Renderer.h"
#include "Rendering/RenderingFactory.h"
#include "UnitTestShaderID.h"

namespace
{
	// Test-local vertex type. Must not be named `Vertex`: the framework already
	// defines a global `Vertex` (ShapeManager.h) with a different layout, and a
	// second definition would be an ODR violation that silently corrupts the heap.
	struct IndexBufferTestVertex
	{
		glm::vec2 Position;
		glm::vec3 Color;
	};

	const std::vector<IndexBufferTestVertex> Vertices = {
			{ glm::vec2(-0.5f, -0.5f), glm::vec3(1.0f, 0.0f, 0.0f) },
			{ glm::vec2(0.5f, -0.5f), glm::vec3(0.0f, 1.0f, 0.0f) },
			{ glm::vec2(0.5f, 0.5f), glm::vec3(0.0f, 0.0f, 1.0f) },
			{ glm::vec2(-0.5f, 0.5f), glm::vec3(1.0f, 1.0f, 1.0f) }
	};

	const std::vector<uint16_t> Indices = { 0, 1, 2, 2, 3, 0 };
}

void IndexBufferTest::Init()
{
	VBuffer = RenderingFactory::Instance()->CreateVertexBuffer();

	VBuffer->SetDataLayout({
		VertexFormat::Vec2, // NDC Coords 
		VertexFormat::Vec3  // Color
						   });

	VBuffer->AddData(Vertices.data(), static_cast<unsigned int>(Vertices.size() * sizeof(IndexBufferTestVertex)));

	IBuffer = RenderingFactory::Instance()->CreateIndexBuffer();
	IBuffer->AddData(Indices.data(), static_cast<unsigned int>(Indices.size()), IndexType::UInt16);

	BasicShader = RenderingFactory::Instance()->CreateShader();
	BasicShader->Init(static_cast<int32_t>(UnitTestShaderID::INDEXBUFFER_TEST));
}

void IndexBufferTest::Close()
{
	delete BasicShader;
	delete VBuffer;
	delete IBuffer;
}

void IndexBufferTest::Update()
{
	Renderer::Instance()->SetCullingFace(CullFace::None);
	Renderer::Instance()->Draw(VBuffer, IBuffer, BasicShader);
}

void IndexBufferTest::RenderObject(class Object* object, class Shader* shader)
{
}

void IndexBufferTest::DebugDisplay()
{
}
