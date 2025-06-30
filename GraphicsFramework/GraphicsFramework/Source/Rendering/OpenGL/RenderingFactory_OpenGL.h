#pragma once

#include "Core/Core.h"
#include "Rendering/RenderingFactory.h"

class RenderingFactory_OpenGL : public RenderingFactory
{
public:
	virtual std::string GetApiName() { return "OPENGL"; }

	virtual ComputeShader* CreateComputeShader();
	virtual IndexBuffer* CreateIndexBuffer();
	virtual FrameBuffer* CreateFrameBuffer();
	virtual Shader* CreateShader();
	virtual ShaderStorageBuffer* CreateShaderStorageBuffer();
	virtual Texture* CreateTexture();
	virtual UniformBuffer* CreateUniformBuffer();
	virtual VertexBuffer* CreateVertexBuffer();
};