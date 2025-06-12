/* Start Header -------------------------------------------------------
Copyright (C) 2018-2019 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the prior
written consent of DigiPen Institute of Technology is prohibited.
Author: Sidhant Tumma
- End Header --------------------------------------------------------*/

#pragma once
#include "Renderer_OpenGL.h"
// Core
#include "Core/Window.h"
#include "Core/ObjectManager.h"
#include "Core/ShapeManager.h"
#include "Core/Components/Transform.h"
#include "Core/Components/Shape.h"
#include "Core/Components/Material.h"
#include "Core/Object.h"
// Rendering base
#include "Rendering/VertexArray.h"
#include "Rendering/ElementArrayBuffer.h"
#include "Rendering/VertexBuffer.h"
#include "Rendering/Shader.h"
#include "Rendering/Texture.h"
#include "Rendering/RenderingFactory.h"
// Rendering OpenGL
#include "Rendering/OpenGL/FrameBuffer_OpenGL.h"
#include "Rendering/OpenGL/VertexArray_OpenGL.h"
#include "Rendering/OpenGL/ElementArrayBuffer_OpenGL.h"
#include "Rendering/OpenGL/VertexBuffer_OpenGL.h"
#include "Rendering/OpenGL/UniformBuffer_OpenGL.h"
#include "Rendering/OpenGL/Shader_OpenGL.h"
#include "Rendering/OpenGL/ComputeShader_OpenGL.h"
#include "Rendering/OpenGL/Texture_OpenGL.h"

class VertexArray;
class VertexBuffer;
class ElementArrayBuffer;

extern Shader *gpShader;
extern Shader *gdShader;

void GLClearError()
{
	while (glGetError() != GL_NO_ERROR);
}

bool GLLogCall(const char* function, const char* file, int line)
{
	while (GLenum error = glGetError())
	{
		std::cout << "[OpenGl Error] (" << std::hex << error << std::dec << "):" << function << " " << file << ":" << line << std::endl;
		return false;
	}
	return true;
}

void Renderer_OpenGL::Init()
{
	//Initialize GLEW
	if (glewInit() != GLEW_OK)
	{
		printf("Error initializing GLEW\n");
	}

	GLCall(glEnable(GL_DEPTH_TEST));
	GLCall(glBlendFunc(GL_ONE, GL_ONE));

	BackBuffer = RenderingFactory::Instance()->CreateFrameBuffer();
	FrameBuffer_OpenGL* BackBufferOpenGl = static_cast<FrameBuffer_OpenGL*>(BackBuffer);
	BackBufferOpenGl->mWidth = Window::Instance()->GetWidth();
	BackBufferOpenGl->mHeight = Window::Instance()->GetHeight();
}

void Renderer_OpenGL::Clear() const
{
	GLCall(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
	glClearColor(mClearColor.x, mClearColor.y, mClearColor.z, 1.0f);
}

void Renderer_OpenGL::SwapBuffers()
{
	SDL_GL_SwapWindow(Window::Instance()->GetWindow());
}

void Renderer_OpenGL::BindShader(const Shader* shader)
{
	GLCall(glUseProgram(static_cast<const Shader_OpenGL*>(shader)->m_RendererID));
}

void Renderer_OpenGL::BindComputeShader(const ComputeShader* shader)
{
	GLCall(glUseProgram(static_cast<const ComputeShader_OpenGL*>(shader)->m_RendererID));
}

void Renderer_OpenGL::BindFrameBuffer(const FrameBuffer* framebuffer)
{
	const FrameBuffer_OpenGL* frameBufferOpenGL = static_cast<const FrameBuffer_OpenGL*>(framebuffer);
	glViewport(0, 0, frameBufferOpenGL->mWidth, frameBufferOpenGL->mHeight);
	GLCall(glBindFramebuffer(GL_FRAMEBUFFER, frameBufferOpenGL->m_RendererID));
}

void Renderer_OpenGL::BindVertexArray(const VertexArray* vertexArray)
{
	GLCall(glBindVertexArray(static_cast<const VertexArray_OpenGL*>(vertexArray)->m_RendererID));
}

void Renderer_OpenGL::BindVertexBuffer(const VertexBuffer* vertexBuffer)
{
	GLCall(glBindBuffer(GL_ARRAY_BUFFER, static_cast<const VertexBuffer_OpenGL*>(vertexBuffer)->m_RendererID));
}

void Renderer_OpenGL::BindElementArrayBuffer(const ElementArrayBuffer* elementArrayBuffer)
{
	GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, static_cast<const ElementArrayBuffer_OpenGL*>(elementArrayBuffer)->m_RendererID));
}

void Renderer_OpenGL::BindUniformBuffer(const UniformBuffer* uniformBuffer, unsigned int bindpoint)
{
	GLCall(glBindBufferBase(GL_UNIFORM_BUFFER, bindpoint, static_cast<const UniformBuffer_OpenGL*>(uniformBuffer)->m_RendererId));
}

void Renderer_OpenGL::BindTexture(const Texture* texture, unsigned int slot)
{
	GLCall(glActiveTexture(GL_TEXTURE0 + slot));
	GLCall(glBindTexture(GL_TEXTURE_2D, static_cast<const Texture_OpenGL*>(texture)->mRendererID));
}

void Renderer_OpenGL::UnbindShader(const Shader* shader)
{
	GLCall(glUseProgram(0));
}

void Renderer_OpenGL::UnbindComputeShader(const ComputeShader* shader)
{
	GLCall(glUseProgram(0));
}

void Renderer_OpenGL::UnbindFrameBuffer(const FrameBuffer* framebuffer)
{
	GLCall(glBindFramebuffer(GL_FRAMEBUFFER, 0));
}

void Renderer_OpenGL::UnbindVertexArray(const VertexArray* vertexArray)
{
	GLCall(glBindVertexArray(0));
}

void Renderer_OpenGL::UnbindVertexBuffer(const VertexBuffer* vertexBuffer)
{
	GLCall(glBindBuffer(GL_ARRAY_BUFFER, 0));
}

void Renderer_OpenGL::UnbindElementArrayBuffer(const ElementArrayBuffer* elementArrayBuffer)
{
	GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
}

void Renderer_OpenGL::UnbindUniformBuffer(const UniformBuffer* uniformBuffer)
{
	GLCall(glBindBuffer(GL_UNIFORM_BUFFER, 0));
}

void Renderer_OpenGL::UnbindTexture(const Texture* texture, unsigned int slot)
{
	GLCall(glActiveTexture(GL_TEXTURE0 + slot));
	GLCall(glBindTexture(GL_TEXTURE_2D, 0));
}

void Renderer_OpenGL::Draw(const VertexArray* va, const ElementArrayBuffer* ib, const Shader* shader)
{
	BindShader(shader);
	BindVertexArray(va);
	BindElementArrayBuffer(ib);
	GLCall(glDrawElements(GL_TRIANGLES, ib->GetCount(), GL_UNSIGNED_INT, nullptr));
	UnbindVertexArray(va);
	UnbindElementArrayBuffer(ib);
}

void Renderer_OpenGL::DebugDraw(const VertexArray* va, const ElementArrayBuffer* ib, const Shader* shader)
{
	BindShader(shader);
	BindVertexArray(va);
	BindElementArrayBuffer(ib);
	GLCall(glDrawElements(GL_LINE_LOOP, ib->GetCount(), GL_UNSIGNED_INT, nullptr));
	UnbindVertexArray(va);
	UnbindElementArrayBuffer(ib);
}

void Renderer_OpenGL::DebugDrawLines(const VertexArray* va, const ElementArrayBuffer* ib, const Shader* shader)
{
	GLCall(glPolygonMode(GL_FRONT_AND_BACK, GL_LINE));
	BindShader(shader);
	BindVertexArray(va);
	BindElementArrayBuffer(ib);
	GLCall(glDrawElements(GL_LINES, ib->GetCount(), GL_UNSIGNED_INT, nullptr));
	UnbindVertexArray(va);
	UnbindElementArrayBuffer(ib);
	GLCall(glPolygonMode(GL_FRONT_AND_BACK, GL_FILL));
}

void Renderer_OpenGL::DrawDebugCircle(const VertexArray* va, const Shader* shader)
{
	BindShader(shader);
	BindVertexArray(va);
	GLCall(glDrawArrays(GL_LINE_LOOP, 0, 16));
	UnbindVertexArray(va);
}

void Renderer_OpenGL::DrawDebugLine(const VertexArray* va, const Shader* shader)
{
	BindShader(shader);
	BindVertexArray(va);
	GLCall(glDrawArrays(GL_LINE_LOOP, 0, 2));
	UnbindVertexArray(va);
}

void Renderer_OpenGL::DrawQuad()
{
	//Quad
	float positions_quad[] = {
			-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
			 1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
			 1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
			-1.0f,  1.0f, 0.0f, 0.0f, 1.0f
	};

	unsigned int indices_quad[] = {
		0, 1, 2,
		2, 3, 0
	};

	VertexArray* VAO_quad = RenderingFactory::Instance()->CreateVertexArray();
	VertexBuffer* VBO_quad = RenderingFactory::Instance()->CreateVertexBuffer();
	ElementArrayBuffer* IBO_quad = RenderingFactory::Instance()->CreateElementArrayBuffer();

	VBO_quad->AddData(positions_quad, 5 * 4 * sizeof(float));
	IBO_quad->AddData(indices_quad, 6, sizeof(unsigned int));
			
	VAO_quad->AddBuffer(VBO_quad);
	VAO_quad->Push(3, GL_FLOAT, sizeof(float));
	VAO_quad->Push(2, GL_FLOAT, sizeof(float));
	VAO_quad->AddLayout();
	UnbindVertexBuffer(VBO_quad);
	BindElementArrayBuffer(IBO_quad);

	GLCall(glDrawElements(GL_TRIANGLES, IBO_quad->GetCount(), GL_UNSIGNED_INT, nullptr));
	UnbindVertexArray(VAO_quad);

	delete VAO_quad;
	delete VBO_quad;
	delete IBO_quad;
}

// Debug rendering code
/*void Renderer_OpenGL::DrawDebugObjects(Shader* shader)
{
	BindShader(shader);
	float width;
	glGetFloatv(GL_LINE_WIDTH,&width);
	for(DebugData& data : mDebugRenderData)
	{
		shader->SetUniform3f("diffuse", data.mColor);
		shader->SetUniform1i("lighting", false);
		shader->SetUniformMat4f("model", data.mModelMatrix);
		glLineWidth(5.0f);
		BindVertexArray(data.mVAO);
		BindElementArrayBuffer(data.mEBO);
		GLCall(glDrawElements(data.mDrawCommand, data.mEBO->GetCount(), GL_UNSIGNED_INT, nullptr));
		UnbindVertexArray(data.mVAO);
		UnbindElementArrayBuffer(data.mEBO);
	}
	mDebugRenderData.clear();
	glLineWidth(width);
	UnbindShader(shader);
}*/

const FrameBuffer* Renderer_OpenGL::GetBackBuffer()
{
	return BackBuffer;
}
