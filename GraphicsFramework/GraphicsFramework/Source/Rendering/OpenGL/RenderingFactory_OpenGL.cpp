#include "RenderingFactory_OpenGL.h"

#include "Rendering/OpenGL/ComputeShader_OpenGL.h"
#include "Rendering/OpenGL/IndexBuffer_OpenGL.h"
#include "Rendering/OpenGL/FrameBuffer_OpenGL.h"
#include "Rendering/OpenGL/Shader_OpenGL.h"
#include "Rendering/OpenGL/ShaderStorageBuffer_OpenGL.h"
#include "Rendering/OpenGL/Texture_OpenGL.h"
#include "Rendering/OpenGL/UniformBuffer_OpenGL.h"
#include "Rendering/OpenGL/VertexBuffer_OpenGL.h"

ComputeShader* RenderingFactory_OpenGL::CreateComputeShader()
{
	return new ComputeShader_OpenGL();
}

IndexBuffer* RenderingFactory_OpenGL::CreateIndexBuffer()
{
	return new IndexBuffer_OpenGL();
}

FrameBuffer* RenderingFactory_OpenGL::CreateFrameBuffer()
{
	return new FrameBuffer_OpenGL();
}

Shader* RenderingFactory_OpenGL::CreateShader()
{
	return new Shader_OpenGL();
}

ShaderStorageBuffer* RenderingFactory_OpenGL::CreateShaderStorageBuffer()
{
	return new ShaderStorageBuffer_OpenGL();
}

Texture* RenderingFactory_OpenGL::CreateTexture()
{
	return new Texture_OpenGL();
}

UniformBuffer* RenderingFactory_OpenGL::CreateUniformBuffer()
{
	return new UniformBuffer_OpenGL();
}

VertexBuffer* RenderingFactory_OpenGL::CreateVertexBuffer()
{
	return new VertexBuffer_OpenGL();
}
