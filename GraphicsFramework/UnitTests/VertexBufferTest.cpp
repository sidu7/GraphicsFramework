#include "VertexBufferTest.h"

#include "Core/Core.h"
#include "Rendering/Renderer.h"
#include "Rendering/RenderingFactory.h"
#include "Rendering/ShaderManager.h"

struct Vertex
{
	glm::vec2 Position;
	glm::vec3 Color;
};

std::vector<Vertex> Vertices = {
	{ glm::vec2(0.0f, -0.5f), glm::vec3(1.0f, 0.0f, 0.0f) },
	{ glm::vec2(0.5f, 0.5f), glm::vec3(0.0f, 1.0f, 0.0f) },
	{ glm::vec2(-0.5f, 0.5f), glm::vec3(0.0f, 0.0f, 1.0f) },
};

void VertexBufferTest::Init()
{
	VBuffer = RenderingFactory::Instance()->CreateVertexBuffer();

	VBuffer->SetDataLayout({ 
		VertexFormat::Vec2, // NDC Coords 
		VertexFormat::Vec3  // Color
						   });

	VBuffer->AddData(Vertices.data(), Vertices.size() * sizeof(Vertex));

	BasicShader = RenderingFactory::Instance()->CreateShader();
	BasicShader->Uses(VBuffer);
	BasicShader->Init("VERTEXBUFFER_TEST");

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
		for (Vertex& Vert : Vertices)
		{
			Vert.Color = glm::vec3(glm::linearRand(0.0f, 1.0f), glm::linearRand(0.0f, 1.0f), glm::linearRand(0.0f, 1.0f));
		}
		VBuffer->AddData(Vertices.data(), Vertices.size() * sizeof(Vertex));
	}
	Renderer::Instance()->Draw(BasicShader, VBuffer, Vertices.size(), 0);
}

void VertexBufferTest::RenderObject(class Object* object, class Shader* shader)
{
}

void VertexBufferTest::DebugDisplay()
{
}