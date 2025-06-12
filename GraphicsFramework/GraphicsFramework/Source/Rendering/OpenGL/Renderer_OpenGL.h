/* Start Header -------------------------------------------------------
Copyright (C) 2018-2019 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the prior
written consent of DigiPen Institute of Technology is prohibited.
Author: Sidhant Tumma
- End Header --------------------------------------------------------*/

#pragma once

#include "Rendering/Renderer.h"

class RenderingObject
{
public:
	virtual inline unsigned int GetId() = 0;
};

class Renderer_OpenGL : public Renderer
{
public:
	virtual void Init() override;
	virtual void Clear() const override;
	virtual void SwapBuffers() override;

	// Bind Functions
	virtual void BindShader(const Shader* shader) override;
	virtual void BindComputeShader(const ComputeShader* shader) override;
	virtual void BindFrameBuffer(const FrameBuffer* framebuffer) override;
	virtual void BindVertexArray(const VertexArray* vertexArray) override;
	virtual void BindVertexBuffer(const VertexBuffer* vertexBuffer) override;
	virtual void BindElementArrayBuffer(const ElementArrayBuffer* elementArrayBuffer) override;
	virtual void BindUniformBuffer(const UniformBuffer* uniformBuffer, unsigned int bindpoint = 0) override;
	virtual void BindTexture(const Texture* texture, unsigned int slot = 0) override;
	
	// Unbind Functions
	virtual void UnbindShader(const Shader* shader) override;
	virtual void UnbindComputeShader(const ComputeShader* shader) override;
	virtual void UnbindFrameBuffer(const FrameBuffer* framebuffer) override;
	virtual void UnbindVertexArray(const VertexArray* vertexArray) override;
	virtual void UnbindVertexBuffer(const VertexBuffer* vertexBuffer) override;
	virtual void UnbindElementArrayBuffer(const ElementArrayBuffer* elementArrayBuffer) override;
	virtual void UnbindUniformBuffer(const UniformBuffer* uniformBuffer) override;
	virtual void UnbindTexture(const Texture* texture, unsigned int slot = 0) override;

	// Draw Functions
	virtual void Draw(const VertexArray* va, const ElementArrayBuffer* ib, const Shader* shader) override;
	virtual void DebugDraw(const VertexArray* va, const ElementArrayBuffer* ib, const Shader* shader) override;
	virtual void DebugDrawLines(const VertexArray * va, const ElementArrayBuffer* ib, const Shader * shader) override;
	virtual void DrawDebugCircle(const VertexArray* va, const Shader* shader) override;
	virtual void DrawDebugLine(const VertexArray* va, const Shader* shader) override;
	virtual void DrawQuad() override;

	virtual const FrameBuffer* GetBackBuffer() override;
protected:
	FrameBuffer* BackBuffer;
};