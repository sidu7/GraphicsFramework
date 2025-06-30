#include "Renderer.h"

#include "Core/Core.h"
#if defined(RENDERER_OPENGL)
#include "Rendering/OpenGL/Renderer_OpenGL.h"
#elif defined(RENDERER_VULKAN)
#include "Rendering/Vulkan/Renderer_Vulkan.h"
#elif defined(RENDERER_DX12)
#endif

#include "RenderingFactory.h"

Renderer* RendererFactory::Create()
{
#if defined(RENDERER_OPENGL)
	return new Renderer_OpenGL();
#elif defined(RENDERER_VULKAN)
	return new Renderer_Vulkan();
#elif defined(RENDERER_DX12)
#endif
	return nullptr;
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

	VertexBuffer* VBO_quad = RenderingFactory::Instance()->CreateVertexBuffer();
	IndexBuffer* IBO_quad = RenderingFactory::Instance()->CreateIndexBuffer();

	VBO_quad->SetDataLayout({ VertexFormat::Vec3, VertexFormat::Vec2 });

	VBO_quad->AddData(positions_quad, 5 * 4 * sizeof(float));
	IBO_quad->AddData(indices_quad, 6, IndexType::UInt16);

	Draw(VBO_quad, IBO_quad, shader);

	delete VBO_quad;
	delete IBO_quad;
}