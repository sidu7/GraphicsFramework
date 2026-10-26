#include "Renderer.h"

#include "RHI/Backend.h"

#include "Rendering/IndexBuffer.h"
#include "Rendering/VertexBuffer.h"

Renderer* Renderer::Instance()
{
	static Renderer* instance = rhi::CreateDevice();
	return instance;
}

void Renderer::DrawQuad(const Shader* shader)
{
	//Quad
	float positions_quad[] = {
			-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
			 1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
			 1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
			-1.0f,  1.0f, 0.0f, 0.0f, 1.0f
	};

	uint16_t indices_quad[] = {
		0, 1, 2,
		2, 3, 0
	};

	VertexBuffer* VBO_quad = Renderer::Instance()->CreateVertexBuffer();
	IndexBuffer* IBO_quad = Renderer::Instance()->CreateIndexBuffer();

	VBO_quad->SetDataLayout({ VertexFormat::Vec3, VertexFormat::Vec2 });

	VBO_quad->AddData(positions_quad, 5 * 4 * sizeof(float));
	IBO_quad->AddData(indices_quad, 6, IndexType::UInt16);

	Draw(VBO_quad, IBO_quad, shader);

	delete VBO_quad;
	delete IBO_quad;
}
