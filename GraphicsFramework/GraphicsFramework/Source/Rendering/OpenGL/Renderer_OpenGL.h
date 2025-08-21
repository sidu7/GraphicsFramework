/* Start Header -------------------------------------------------------
Copyright (C) 2018-2019 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the prior
written consent of DigiPen Institute of Technology is prohibited.
Author: Sidhant Tumma
- End Header --------------------------------------------------------*/

#pragma once

#include "Rendering/Renderer.h"

#define ASSERT(x) if (!(x)) __debugbreak();
#define GLCall(x) GLClearError();\
	x;\
	ASSERT(GLLogCall(#x,__FILE__,__LINE__))


void GLClearError();

bool GLLogCall(const char* function, const char* file, int line);

class Renderer_OpenGL : public Renderer
{
public:
	virtual void Init() override;
	virtual void Close() override;
	
	virtual void StartFrame() override;
	virtual void SwapBuffers() override;

	virtual void WaitFrameFinish() const override {}
	virtual void Clear() const override;

	// Bind Functions
	virtual void BindShader(const Shader* shader) override;
	virtual void BindComputeShader(const ComputeShader* shader) override;
	virtual void BindFrameBuffer(const FrameBuffer* framebuffer) override;
	virtual void BindVertexBuffer(const VertexBuffer* vertexBuffer) override;
	virtual void BindIndexBuffer(const IndexBuffer* elementArrayBuffer) override;
	virtual void BindUniformBuffer(const UniformBuffer* uniformBuffer, unsigned int bindpoint = 0) override;
	virtual void BindTexture(const Texture* texture, unsigned int slot = 0) override;
	
	// Unbind Functions
	virtual void UnbindShader(const Shader* shader) override;
	virtual void UnbindComputeShader(const ComputeShader* shader) override;
	virtual void UnbindFrameBuffer(const FrameBuffer* framebuffer) override;
	virtual void UnbindVertexBuffer(const VertexBuffer* vertexBuffer) override;
	virtual void UnbindIndexBuffer(const IndexBuffer* indexBuffer) override;
	virtual void UnbindUniformBuffer(const UniformBuffer* uniformBuffer) override;
	virtual void UnbindTexture(const Texture* texture, unsigned int slot = 0) override;

	// Draw Functions
	virtual void Draw(const Shader* shader, const VertexBuffer* vertexBuffer, int32_t vertexCount, int32_t vertexStart, int32_t instanceCount = 1, int32_t instanceStart = 0) override;
	virtual void Draw(const VertexBuffer* vertexBuffer, const IndexBuffer* indexBuffer, const Shader* shader) override;
	virtual void DebugDraw(const VertexBuffer* vertexBuffer, const IndexBuffer* indexBuffer, const Shader* shader) override;
	virtual void DebugDrawLines(const VertexBuffer* vertexBuffer, const IndexBuffer* indexBuffer, const Shader * shader) override;
	virtual void DrawDebugCircle(const VertexBuffer* vertexBuffer, const Shader* Shader) override;
	virtual void DrawDebugLine(const VertexBuffer* vertexBuffer, const Shader* shader) override;

	// Rendering Features
	virtual void SetDepthTest(bool bEnabled) override;
	virtual void SetBlending(bool bEnabled) override;
	virtual void SetCullingFace(CullFace Face) override;
	virtual CullFace GetCullingFace() override;

	virtual const FrameBuffer* GetBackBuffer() override;

	virtual void SetViewportSize(Rect3D viewport) override;
	virtual void SetScissorSize(Rect2D scissor) override;

	virtual Rect3D GetViewportSize() override { return ViewportSize; }
	virtual Rect2D GetScissorSize() override { return ScissorSize; }

protected:
	FrameBuffer* BackBuffer;
	CullFace CullingFace;

	Rect3D ViewportSize;
	Rect2D ScissorSize;
};