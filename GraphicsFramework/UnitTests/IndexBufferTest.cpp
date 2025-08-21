#include "IndexBufferTest.h"

#include "Core/Core.h"
#include "Rendering/Renderer.h"
#include "Rendering/RenderingFactory.h"
#include "Rendering/ShaderManager.h"

struct Vertex
{
	glm::vec2 Position;
	glm::vec3 Color;
};

const std::vector<Vertex> Vertices = {
		{ glm::vec2(-0.5f, -0.5f), glm::vec3(1.0f, 0.0f, 0.0f) },
		{ glm::vec2(0.5f, -0.5f), glm::vec3(0.0f, 1.0f, 0.0f) },
		{ glm::vec2(0.5f, 0.5f), glm::vec3(0.0f, 0.0f, 1.0f) },
		{ glm::vec2(-0.5f, 0.5f), glm::vec3(1.0f, 1.0f, 1.0f) }
};

const std::vector<uint16_t> Indices = { 0, 1, 2, 2, 3, 0 };

void IndexBufferTest::Init()
{
	VBuffer = RenderingFactory::Instance()->CreateVertexBuffer();

	VBuffer->SetDataLayout({
		VertexFormat::Vec2, // NDC Coords 
		VertexFormat::Vec3  // Color
						   });

	VBuffer->AddData(Vertices.data(), Vertices.size() * sizeof(Vertex));

	IBuffer = RenderingFactory::Instance()->CreateIndexBuffer();
	IBuffer->AddData(Indices.data(), Indices.size(), IndexType::UInt16);

	BasicShader = RenderingFactory::Instance()->CreateShader();
	BasicShader->Init("INDEXBUFFER_TEST");
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