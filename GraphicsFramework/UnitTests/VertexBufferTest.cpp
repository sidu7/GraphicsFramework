#include "VertexBufferTest.h"

#include "Core/Core.h"
#include "Rendering/Shader.h"
#include "Rendering/VertexBuffer.h"
#include "Rendering/Renderer.h"
#include "UnitTestShaderID.h"

namespace
{
	// Test-local vertex type. Must not be named `Vertex`: the framework already
	// defines a global `Vertex` (ShapeManager.h) with a different layout, and a
	// second definition would be an ODR violation that silently corrupts the heap.
	struct VertexBufferTestVertex
	{
		glm::vec2 Position;
		glm::vec3 Color;
	};

	std::vector<VertexBufferTestVertex> Vertices = {
		{ glm::vec2(0.0f, -0.5f), glm::vec3(1.0f, 0.0f, 0.0f) },
		{ glm::vec2(0.5f, 0.5f), glm::vec3(0.0f, 1.0f, 0.0f) },
		{ glm::vec2(-0.5f, 0.5f), glm::vec3(0.0f, 0.0f, 1.0f) },
	};
}

void VertexBufferTest::Init()
{
	VBuffer = Renderer::Instance()->CreateVertexBuffer();

	VBuffer->SetDataLayout({ 
		VertexFormat::Vec2, // NDC Coords 
		VertexFormat::Vec3  // Color
						   });

	VBuffer->AddData(Vertices.data(), static_cast<unsigned int>(Vertices.size() * sizeof(VertexBufferTestVertex)));

	BasicShader = Renderer::Instance()->CreateShader();
	BasicShader->Init(static_cast<int32_t>(UnitTestShaderID::VERTEXBUFFER_TEST));

	TestTime = 0.0f;
}

void VertexBufferTest::Close()
{
	delete BasicShader;
	delete VBuffer;
}

void VertexBufferTest::Update()
{
	TestTime += Time::Instance()->deltaTime;

	if (TestTime > 0.5f)
	{
		TestTime = 0.0f;
		for (VertexBufferTestVertex& Vert : Vertices)
		{
			Vert.Color = glm::vec3(glm::linearRand(0.0f, 1.0f), glm::linearRand(0.0f, 1.0f), glm::linearRand(0.0f, 1.0f));
		}
		VBuffer->AddData(Vertices.data(), static_cast<unsigned int>(Vertices.size() * sizeof(VertexBufferTestVertex)));
	}
	Renderer::Instance()->Draw(BasicShader, VBuffer, static_cast<int32_t>(Vertices.size()), 0);
}

void VertexBufferTest::RenderObject(class Object* object, class Shader* shader)
{
}

void VertexBufferTest::DebugDisplay()
{
}
