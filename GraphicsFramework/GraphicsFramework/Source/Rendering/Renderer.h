/* Start Header -------------------------------------------------------
Copyright (C) 2018-2019 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the prior
written consent of DigiPen Institute of Technology is prohibited.
Author: Sidhant Tumma
- End Header --------------------------------------------------------*/

#pragma once

#include "GLEW/include/GL/glew.h"
#include "Core/Singleton.h"
#include "SDL/include/SDL.h"
#include <vector>
#include <glm/mat4x2.hpp>

class IndexBuffer;
class Shader;
class Object;
class Renderer;
class Transform;
class Shape;
class Material;
class FrameBuffer;
class VertexBuffer;
class ComputeShader;
class UniformBuffer;
class Texture;

class RendererFactory
{
public:
	static Renderer* Create();
};

enum class CullFace
{
	None,
	Front,
	Back,
	Both
};

struct Rect2D
{
	Rect2D() : Offset(0.f), Size(0.f) {}

	glm::vec2 Offset;
	glm::vec2 Size;
};

struct Rect3D : public Rect2D
{
	Rect3D() : Depth(0.0f, 1.0f) {}
	glm::vec2 Depth;
};

class Renderer : public SingletonByFactory<Renderer, RendererFactory>
{
public:
	virtual void Init() = 0;
	virtual void Close() = 0;
	
	virtual void StartFrame() = 0;
	virtual void SwapBuffers() = 0;

	virtual void Clear() const = 0;
	virtual void WaitFrameFinish() const = 0;

	// Bind Functions
	virtual void BindShader(const Shader* shader) = 0;
	virtual void BindComputeShader(const ComputeShader* shader) = 0;
	virtual void BindFrameBuffer(const FrameBuffer* framebuffer) = 0;
	virtual void BindVertexBuffer(const VertexBuffer* vertexBuffer) = 0;
	virtual void BindIndexBuffer(const IndexBuffer* elementArrayBuffer) = 0;
	virtual void BindUniformBuffer(const UniformBuffer* uniformBuffer, unsigned int bindpoint = 0) = 0;
	virtual void BindTexture(const Texture* texture, unsigned int slot = 0) = 0;

	// Unbind Functions
	virtual void UnbindShader(const Shader* shader) = 0;
	virtual void UnbindComputeShader(const ComputeShader* shader) = 0;
	virtual void UnbindFrameBuffer(const FrameBuffer* framebuffer) = 0;
	virtual void UnbindVertexBuffer(const VertexBuffer* vertexBuffer) = 0;
	virtual void UnbindIndexBuffer(const IndexBuffer* elementArrayBuffer) = 0;
	virtual void UnbindUniformBuffer(const UniformBuffer* uniformBuffer) = 0;
	virtual void UnbindTexture(const Texture* texture, unsigned int slot = 0) = 0;

	// Draw Functions
	virtual void Draw(const Shader* Shader, const VertexBuffer* vertexBuffer, int32_t VertexCount, int32_t VertexStart, int32_t InstanceCount = 1, int32_t InstanceStart = 0) = 0;
	virtual void Draw(const VertexBuffer* VertexBuffer, const IndexBuffer* IndexBuffer, const Shader* Shader) = 0;
	virtual void DebugDraw(const VertexBuffer* VertexBuffer, const IndexBuffer* IndexBuffer, const Shader* Shader) = 0;
	virtual void DebugDrawLines(const VertexBuffer* VertexBuffer, const IndexBuffer* IndexBuffer, const Shader* Shader) = 0;
	virtual void DrawDebugCircle(const VertexBuffer* VertexBuffer, const Shader* Shader) = 0;
	virtual void DrawDebugLine(const VertexBuffer* VertexBuffer, const Shader* Shader) = 0;
	virtual void DrawQuad(const Shader* shader);

	// Rendering Features
	virtual void SetDepthTest(bool bEnabled) = 0;
	virtual void SetBlending(bool bEnabled) = 0;
	virtual void SetCullingFace(CullFace Face) = 0;
	virtual CullFace GetCullingFace() = 0;

	virtual void SetClearColor(glm::vec3 color) { mClearColor = color; }
	virtual const FrameBuffer* GetBackBuffer() = 0;

	virtual void SetViewportSize(Rect3D viewport) = 0;
	virtual void SetScissorSize(Rect2D scissor) = 0;

	// x, y - Offset (X x Y) , z, w - Size (W x H)
	virtual Rect3D GetViewportSize() = 0;
	virtual Rect2D GetScissorSize() = 0;

protected:
	glm::vec3 mClearColor;
};