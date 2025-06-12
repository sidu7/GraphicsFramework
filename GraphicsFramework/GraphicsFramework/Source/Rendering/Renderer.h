/* Start Header -------------------------------------------------------
Copyright (C) 2018-2019 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the prior
written consent of DigiPen Institute of Technology is prohibited.
Author: Sidhant Tumma
- End Header --------------------------------------------------------*/

#pragma once

#include "GLEW/include/GL/glew.h"
#include "../core/Singleton.h"
#include "SDL/include/SDL.h"
#include <vector>
#include <glm/mat4x2.hpp>


#define ASSERT(x) if (!(x)) __debugbreak();
#define GLCall(x) GLClearError();\
	x;\
	ASSERT(GLLogCall(#x,__FILE__,__LINE__))


void GLClearError();

bool GLLogCall(const char* function, const char* file, int line);

class VertexArray;
class ElementArrayBuffer;
class Shader;
class Object;
class Renderer;

class RendererFactory
{
public:
	static Renderer* Create();
};

class Transform;
class Shape;
class Material;
class FrameBuffer;
class VertexBuffer;
class ComputeShader;
class UniformBuffer;
class Texture;

class Renderer : public SingletonByFactory<Renderer, RendererFactory>
{
public:
	virtual void Init() = 0;
	virtual void Clear() const = 0;
	virtual void SwapBuffers() = 0;

	// Bind Functions
	virtual void BindShader(const Shader* shader) = 0;
	virtual void BindComputeShader(const ComputeShader* shader) = 0;
	virtual void BindFrameBuffer(const FrameBuffer* framebuffer) = 0;
	virtual void BindVertexArray(const VertexArray* vertexArray) = 0;
	virtual void BindVertexBuffer(const VertexBuffer* vertexBuffer) = 0;
	virtual void BindElementArrayBuffer(const ElementArrayBuffer* elementArrayBuffer) = 0;
	virtual void BindUniformBuffer(const UniformBuffer* uniformBuffer, unsigned int bindpoint = 0) = 0;
	virtual void BindTexture(const Texture* texture, unsigned int slot = 0) = 0;

	// Unbind Functions
	virtual void UnbindShader(const Shader* shader) = 0;
	virtual void UnbindComputeShader(const ComputeShader* shader) = 0;
	virtual void UnbindFrameBuffer(const FrameBuffer* framebuffer) = 0;
	virtual void UnbindVertexArray(const VertexArray* vertexArray) = 0;
	virtual void UnbindVertexBuffer(const VertexBuffer* vertexBuffer) = 0;
	virtual void UnbindElementArrayBuffer(const ElementArrayBuffer* elementArrayBuffer) = 0;
	virtual void UnbindUniformBuffer(const UniformBuffer* uniformBuffer) = 0;
	virtual void UnbindTexture(const Texture* texture, unsigned int slot = 0) = 0;

	// Draw Functions
	virtual void Draw(const VertexArray* va, const ElementArrayBuffer* ib, const Shader* shader) = 0;
	virtual void DebugDraw(const VertexArray* va, const ElementArrayBuffer* ib, const Shader* shader) = 0;
	virtual void DebugDrawLines(const VertexArray* va, const ElementArrayBuffer* ib, const Shader* shader) = 0;
	virtual void DrawDebugCircle(const VertexArray* va, const Shader* shader) = 0;
	virtual void DrawDebugLine(const VertexArray* va, const Shader* shader) = 0;
	virtual void DrawQuad() = 0;

	virtual void SetClearColor(glm::vec3 color) { mClearColor = color; }
	virtual const FrameBuffer* GetBackBuffer() = 0;

protected:
	glm::vec3 mClearColor;
};