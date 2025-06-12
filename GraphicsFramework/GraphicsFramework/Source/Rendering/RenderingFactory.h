#pragma once

#include "Core/Core.h"
#include "Rendering/ComputeShader.h"
#include "Rendering/ElementArrayBuffer.h"
#include "Rendering/FrameBuffer.h"
#include "Rendering/Shader.h"
#include "Rendering/ShaderStorageBuffer.h"
#include "Rendering/Texture.h"
#include "Rendering/UniformBuffer.h"
#include "Rendering/VertexArray.h"
#include "Rendering/VertexBuffer.h"

class RenderingFactory;

class RenderingFactorySelector
{
public:
	static RenderingFactory* Create();
};

class RenderingFactory : public SingletonByFactory<RenderingFactory, RenderingFactorySelector>
{
public:
	virtual ComputeShader* CreateComputeShader() = 0;
	virtual ElementArrayBuffer* CreateElementArrayBuffer() = 0;
	virtual FrameBuffer* CreateFrameBuffer() = 0;
	virtual Shader* CreateShader() = 0;
	virtual ShaderStorageBuffer* CreateShaderStorageBuffer() = 0;
	virtual Texture* CreateTexture() = 0;
	virtual UniformBuffer* CreateUniformBuffer() = 0;
	virtual VertexArray* CreateVertexArray() = 0;
	virtual VertexBuffer* CreateVertexBuffer() = 0;
};