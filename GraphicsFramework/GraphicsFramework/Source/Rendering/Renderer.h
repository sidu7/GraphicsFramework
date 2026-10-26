/* Start Header -------------------------------------------------------
Copyright (C) 2018-2019 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the prior
written consent of DigiPen Institute of Technology is prohibited.
Author: Sidhant Tumma
- End Header --------------------------------------------------------*/

#pragma once

#include <cstdint>

#include "RHI/Types.h"

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
class ShaderStorageBuffer;
class Texture;

// Abstract rendering device.
//
// A Renderer owns the backend context and is the entry point for both resource
// creation and frame submission. The concrete backend (OpenGL / Vulkan) is
// selected at build time; see docs/RHI-Migration-Plan.md.
class Renderer
{
public:
	static Renderer* Instance();

	virtual ~Renderer() = default;

	virtual const char* GetApiName() const = 0;

	virtual void Init() = 0;
	virtual void Close() = 0;
	
	virtual void StartFrame() = 0;
	virtual void SwapBuffers() = 0;

	virtual void Clear() const = 0;
	virtual void WaitFrameFinish() const = 0;

	// Resource creation
	virtual ComputeShader* CreateComputeShader() = 0;
	virtual IndexBuffer* CreateIndexBuffer() = 0;
	virtual FrameBuffer* CreateFrameBuffer() = 0;
	virtual Shader* CreateShader() = 0;
	virtual ShaderStorageBuffer* CreateShaderStorageBuffer() = 0;
	virtual Texture* CreateTexture() = 0;
	virtual UniformBuffer* CreateUniformBuffer() = 0;
	virtual VertexBuffer* CreateVertexBuffer() = 0;

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

	// Backend capability hooks for ImGui (called by ImguiManager)
	virtual void InitImGui() {}
	virtual void ShutdownImGui() {}
	virtual void NewFrame() {}
	virtual void Render() {}

	// Clip-space capability: returns -1 for Vulkan's inverted-Y NDC, 1 for OpenGL
	virtual float GetProjectionYFlip() const { return 1.0f; }

protected:
	glm::vec3 mClearColor;
};
