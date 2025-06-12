#pragma once

#include "Core/Core.h"
#include "Rendering/RenderingFactory.h"

class RenderingFactory_OpenGL : public RenderingFactory
{
public:
	virtual ComputeShader* CreateComputeShader();
	virtual ElementArrayBuffer* CreateElementArrayBuffer();
	virtual FrameBuffer* CreateFrameBuffer();
	virtual Shader* CreateShader();
	virtual ShaderStorageBuffer* CreateShaderStorageBuffer();
	virtual Texture* CreateTexture();
	virtual UniformBuffer* CreateUniformBuffer();
	virtual VertexArray* CreateVertexArray();
	virtual VertexBuffer* CreateVertexBuffer();
};