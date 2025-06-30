#include "UniformBufferTest.h"

#include "Core/Core.h"
#include "Rendering/Renderer.h"
#include "Rendering/RenderingFactory.h"
#include "Rendering/ShaderManager.h"
#include "Core/Window.h"

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

struct Matrices
{
	alignas(16) glm::mat4 Perspective;
	alignas(16) glm::mat4 View;
	alignas(16) glm::mat4 Model;
};

Matrices MatrixData;

void UniformBufferTest::Init()
{
	VBuffer = RenderingFactory::Instance()->CreateVertexBuffer();

	VBuffer->SetDataLayout({
		VertexFormat::Vec2, // NDC Coords 
		VertexFormat::Vec3  // Color
						   });

	VBuffer->AddData(Vertices.data(), Vertices.size() * sizeof(Vertex));

	IBuffer = RenderingFactory::Instance()->CreateIndexBuffer();
	IBuffer->AddData(Indices.data(), Indices.size(), IndexType::UInt16);

	glm::vec2 WindowSize = Window::Instance()->GetWindowSize();
	MatrixData.Perspective = glm::perspective(glm::radians(45.f), WindowSize.x / WindowSize.y, 0.1f, 100.f);
	MatrixData.View = glm::lookAt(glm::vec3(2.f, 2.f, 2.f), glm::vec3(0.f), glm::vec3(0.f, 0.f, 1.f));
	MatrixData.Model = glm::mat4(1.f);

	UBuffer = RenderingFactory::Instance()->CreateUniformBuffer();
	UBuffer->Init(sizeof(Matrices), UboBinding, &MatrixData);

	BasicShader = RenderingFactory::Instance()->CreateShader();
	BasicShader->Uses(VBuffer);
	BasicShader->Uses(IBuffer);
	BasicShader->Uses(UBuffer, UboBinding);
	BasicShader->Init("UNIFORMBUFFER_TEST");

	RunTime = 0.f;
	RotateX = false;
	RotateY = false;
	RotateZ = true;
}

void UniformBufferTest::Close()
{
	delete BasicShader;
	delete VBuffer;
	delete IBuffer;
	delete UBuffer;
}

void UniformBufferTest::Update()
{
	Renderer::Instance()->SetCullingFace(CullFace::None);

	RunTime += Time::Instance()->deltaTime;
	MatrixData.Model = glm::rotate(glm::mat4(1.f), RunTime * glm::radians(90.f), glm::vec3(RotateX, RotateY, RotateZ));

	UBuffer->AddData(sizeof(Matrices), &MatrixData);

	Renderer::Instance()->BindUniformBuffer(UBuffer, UboBinding);

	Renderer::Instance()->Draw(VBuffer, IBuffer, BasicShader);
}

void UniformBufferTest::RenderObject(class Object* object, class Shader* shader)
{
}

void UniformBufferTest::DebugDisplay()
{
	ImGui::Begin("Uniform Buffer Test");
	ImGui::Checkbox("Rotate Around X", &RotateX);
	ImGui::Checkbox("Rotate Around Y", &RotateY);
	ImGui::Checkbox("Rotate Around Z", &RotateZ);
	ImGui::End();
}