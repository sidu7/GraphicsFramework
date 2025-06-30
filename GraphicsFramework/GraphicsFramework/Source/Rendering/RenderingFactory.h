#pragma once

#include "Core/Core.h"
#include "Rendering/ComputeShader.h"
#include "Rendering/IndexBuffer.h"
#include "Rendering/FrameBuffer.h"
#include "Rendering/Shader.h"
#include "Rendering/ShaderStorageBuffer.h"
#include "Rendering/Texture.h"
#include "Rendering/UniformBuffer.h"
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
	virtual std::string GetApiName() = 0;

	virtual void Init() {}
	virtual void Close() {}

	virtual ComputeShader* CreateComputeShader() = 0;
	virtual IndexBuffer* CreateIndexBuffer() = 0;
	virtual FrameBuffer* CreateFrameBuffer() = 0;
	virtual Shader* CreateShader() = 0;
	virtual ShaderStorageBuffer* CreateShaderStorageBuffer() = 0;
	virtual Texture* CreateTexture() = 0;
	virtual UniformBuffer* CreateUniformBuffer() = 0;
	virtual VertexBuffer* CreateVertexBuffer() = 0;
};