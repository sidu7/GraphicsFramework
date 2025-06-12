#include "RenderingFactory_OpenGL.h"

#include "Rendering/OpenGL/ComputeShader_OpenGL.h"
#include "Rendering/OpenGL/ElementArrayBuffer_OpenGL.h"
#include "Rendering/OpenGL/FrameBuffer_OpenGL.h"
#include "Rendering/OpenGL/Shader_OpenGL.h"
#include "Rendering/OpenGL/ShaderStorageBuffer_OpenGL.h"
#include "Rendering/OpenGL/Texture_OpenGL.h"
#include "Rendering/OpenGL/UniformBuffer_OpenGL.h"
#include "Rendering/OpenGL/VertexArray_OpenGL.h"
#include "Rendering/OpenGL/VertexBuffer_OpenGL.h"

ComputeShader* RenderingFactory_OpenGL::CreateComputeShader()
{
	return new ComputeShader_OpenGL();
}

ElementArrayBuffer* RenderingFactory_OpenGL::CreateElementArrayBuffer()
{
	return new ElementArrayBuffer_OpenGL();
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

VertexArray* RenderingFactory_OpenGL::CreateVertexArray()
{
	return new VertexArray_OpenGL();
}

VertexBuffer* RenderingFactory_OpenGL::CreateVertexBuffer()
{
	return new VertexBuffer_OpenGL();
}
