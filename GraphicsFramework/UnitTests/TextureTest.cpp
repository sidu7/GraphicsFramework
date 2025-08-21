#include "TextureTest.h"

#include "Core/Core.h"
#include "Rendering/Renderer.h"
#include "Rendering/RenderingFactory.h"
#include "Rendering/ShaderManager.h"
#include "Core/Window.h"

struct TextureTestMatrices
{
	alignas(16) glm::mat4 Perspective;
	alignas(16) glm::mat4 View;
	alignas(16) glm::mat4 Model;
};

TextureTestMatrices TexTestMatrixData;

struct TextureTestVertex
{
	glm::vec2 Position;
	glm::vec3 Color;
	glm::vec2 TexCoords;
};

const std::vector<TextureTestVertex> Vertices = {
		{ glm::vec2(-0.5f, -0.5f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec2(1.f, 0.f) },
		{ glm::vec2(0.5f, -0.5f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(0.f, 0.f) },
		{ glm::vec2(0.5f, 0.5f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0.f, 1.f) },
		{ glm::vec2(-0.5f, 0.5f), glm::vec3(1.0f, 1.0f, 1.0f), glm::vec2(1.f, 1.f) }
};

const std::vector<uint16_t> Indices = { 0, 1, 2, 2, 3, 0 };

void TextureTest::Init()
{
	pVBuffer = RenderingFactory::Instance()->CreateVertexBuffer();

	pVBuffer->SetDataLayout({
		VertexFormat::Vec2, // NDC Coords 
		VertexFormat::Vec3, // Color
		VertexFormat::Vec2  // Tex Coords
						   });

	pVBuffer->AddData(Vertices.data(), Vertices.size() * sizeof(TextureTestVertex));

	pIBuffer = RenderingFactory::Instance()->CreateIndexBuffer();
	pIBuffer->AddData(Indices.data(), Indices.size(), IndexType::UInt16);

	glm::vec2 WindowSize = Window::Instance()->GetWindowSize();
	TexTestMatrixData.Perspective = glm::perspective(glm::radians(45.f), WindowSize.x / WindowSize.y, 0.1f, 100.f);
	TexTestMatrixData.View = glm::lookAt(glm::vec3(2.f, 2.f, 2.f), glm::vec3(0.f), glm::vec3(0.f, 0.f, 1.f));
	TexTestMatrixData.Model = glm::mat4(1.f);

	pUBuffer = RenderingFactory::Instance()->CreateUniformBuffer();
	pUBuffer->Init(sizeof(TextureTestMatrices), UboBinding, &TexTestMatrixData);

	pTexture = RenderingFactory::Instance()->CreateTexture();
	pTexture->Init("res/Texture.jpg");

	BasicShader = RenderingFactory::Instance()->CreateShader();
	BasicShader->Uses(pVBuffer);
	BasicShader->Uses(pIBuffer);
	BasicShader->Uses(pUBuffer, UboBinding);
	BasicShader->Uses(pTexture, TexBinding);
	BasicShader->Init("TEXTURE_TEST");

	RunTime = 0.f;
	RotateX = false;
	RotateY = false;
	RotateZ = true;
}

void TextureTest::Close()
{
	delete BasicShader;
	delete pVBuffer;
	delete pIBuffer;
	delete pUBuffer;
}

void TextureTest::Update()
{
	Renderer::Instance()->SetCullingFace(CullFace::None);

	glm::vec3 RotateAxis = glm::vec3(RotateX, RotateY, RotateZ);	
	if (glm::length(RotateAxis) > 0.f)
	{
		RunTime += Time::Instance()->deltaTime;
		TexTestMatrixData.Model = glm::rotate(glm::mat4(1.f), RunTime * glm::radians(90.f), RotateAxis);
	}

	pUBuffer->AddData(sizeof(TextureTestMatrices), &TexTestMatrixData);

	Renderer::Instance()->BindTexture(pTexture, TexBinding);
	Renderer::Instance()->BindUniformBuffer(pUBuffer, UboBinding);

	Renderer::Instance()->Draw(pVBuffer, pIBuffer, BasicShader);
}

void TextureTest::RenderObject(class Object* object, class Shader* shader)
{
}

void TextureTest::DebugDisplay()
{
	ImGui::Begin("Texture Buffer Test");
	ImGui::Checkbox("Rotate Around X", &RotateX);
	ImGui::Checkbox("Rotate Around Y", &RotateY);
	ImGui::Checkbox("Rotate Around Z", &RotateZ);
	ImGui::End();
}