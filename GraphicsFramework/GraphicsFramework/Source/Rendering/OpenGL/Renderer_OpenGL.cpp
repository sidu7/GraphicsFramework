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
#include "Rendering/IndexBuffer.h"
#include "Rendering/VertexBuffer.h"
#include "Rendering/Shader.h"
#include "Rendering/Texture.h"
#include "Rendering/RenderingFactory.h"
// Rendering OpenGL
#include "Rendering/OpenGL/FrameBuffer_OpenGL.h"
#include "Rendering/OpenGL/IndexBuffer_OpenGL.h"
#include "Rendering/OpenGL/VertexBuffer_OpenGL.h"
#include "Rendering/OpenGL/UniformBuffer_OpenGL.h"
#include "Rendering/OpenGL/Shader_OpenGL.h"
#include "Rendering/OpenGL/ComputeShader_OpenGL.h"
#include "Rendering/OpenGL/Texture_OpenGL.h"

class VertexArray;
class VertexBuffer;
class IndexBuffer;

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

	CullingFace = CullFace::Back;
}

void Renderer_OpenGL::Close()
{
	delete BackBuffer;
}

void Renderer_OpenGL::Clear() const
{
	glClearColor(mClearColor.x, mClearColor.y, mClearColor.z, 1.0f);
	GLCall(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
}

void Renderer_OpenGL::StartFrame()
{
	Clear();
}

void Renderer_OpenGL::SwapBuffers()
{
	SDL_GL_SwapWindow(Window::Instance()->GetWindow());
}

void Renderer_OpenGL::BindShader(const Shader* shader)
{
	if (shader)
	{
		GLCall(glUseProgram(static_cast<const Shader_OpenGL*>(shader)->m_RendererID));
	}
}

void Renderer_OpenGL::BindComputeShader(const ComputeShader* shader)
{
	if (shader)
	{
		GLCall(glUseProgram(static_cast<const ComputeShader_OpenGL*>(shader)->m_RendererID));
	}
}

void Renderer_OpenGL::BindFrameBuffer(const FrameBuffer* framebuffer)
{
	if (framebuffer)
	{
		const FrameBuffer_OpenGL* frameBufferOpenGL = static_cast<const FrameBuffer_OpenGL*>(framebuffer);
		glViewport(0, 0, frameBufferOpenGL->mWidth, frameBufferOpenGL->mHeight);
		GLCall(glBindFramebuffer(GL_FRAMEBUFFER, frameBufferOpenGL->m_RendererID));
	}
}

void Renderer_OpenGL::BindVertexBuffer(const VertexBuffer* vertexBuffer)
{
	if (vertexBuffer)
	{
		GLCall(glBindVertexArray(static_cast<const VertexBuffer_OpenGL*>(vertexBuffer)->m_VertexArrayID));
		GLCall(glBindBuffer(GL_ARRAY_BUFFER, static_cast<const VertexBuffer_OpenGL*>(vertexBuffer)->m_VertexBufferID));
	}
}

void Renderer_OpenGL::BindIndexBuffer(const IndexBuffer* indexBuffer)
{
	if (indexBuffer)
	{
		GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, static_cast<const IndexBuffer_OpenGL*>(indexBuffer)->m_RendererID));
	}
}

void Renderer_OpenGL::BindUniformBuffer(const UniformBuffer* uniformBuffer, unsigned int bindpoint)
{
	if (uniformBuffer)
	{
		GLCall(glBindBufferBase(GL_UNIFORM_BUFFER, bindpoint, static_cast<const UniformBuffer_OpenGL*>(uniformBuffer)->m_RendererId));
	}
}

void Renderer_OpenGL::BindTexture(const Texture* texture, unsigned int slot)
{
	if (texture)
	{
		GLCall(glActiveTexture(GL_TEXTURE0 + slot));
		GLCall(glBindTexture(GL_TEXTURE_2D, static_cast<const Texture_OpenGL*>(texture)->mRendererID));
	}
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

void Renderer_OpenGL::UnbindVertexBuffer(const VertexBuffer* vertexBuffer)
{
	GLCall(glBindVertexArray(0));
	GLCall(glBindBuffer(GL_ARRAY_BUFFER, 0));
}

void Renderer_OpenGL::UnbindIndexBuffer(const IndexBuffer* elementArrayBuffer)
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

void Renderer_OpenGL::Draw(const Shader* Shader, const VertexBuffer* vertexBuffer, int32_t VertexCount, int32_t VertexStart, int32_t InstanceCount, int32_t InstanceStart)
{
	BindShader(Shader);
	VertexBuffer* Vb = vertexBuffer ? const_cast<VertexBuffer*>(vertexBuffer) : RenderingFactory::Instance()->CreateVertexBuffer();
	BindVertexBuffer(Vb);
	GLCall(glDrawArrays(GL_TRIANGLES, 0, VertexCount));
	UnbindVertexBuffer(Vb);
	UnbindShader(Shader);
	if (!vertexBuffer)
	{
		delete Vb;
	}
}
void Renderer_OpenGL::Draw(const VertexBuffer* vertexBuffer, const IndexBuffer* indexBuffer, const Shader* shader)
{
	if (indexBuffer)
	{
		BindShader(shader);
		BindVertexBuffer(vertexBuffer);
		BindIndexBuffer(indexBuffer);
		GLCall(glDrawElements(GL_TRIANGLES, indexBuffer->GetCount(), GL_UNSIGNED_SHORT, nullptr));
		UnbindVertexBuffer(vertexBuffer);
		UnbindIndexBuffer(indexBuffer);
	}
}

void Renderer_OpenGL::DebugDraw(const VertexBuffer* vertexBuffer, const IndexBuffer* indexBuffer, const Shader* shader)
{
	if (indexBuffer)
	{
		BindShader(shader);
		BindVertexBuffer(vertexBuffer);
		BindIndexBuffer(indexBuffer);
		GLCall(glDrawElements(GL_LINE_LOOP, indexBuffer->GetCount(), GL_UNSIGNED_SHORT, nullptr));
		UnbindVertexBuffer(vertexBuffer);
		UnbindIndexBuffer(indexBuffer);
	}
}

void Renderer_OpenGL::DebugDrawLines(const VertexBuffer* vertexBuffer, const IndexBuffer* indexBuffer, const Shader* shader)
{
	if (indexBuffer)
	{
		GLCall(glPolygonMode(GL_FRONT_AND_BACK, GL_LINE));
		BindShader(shader);
		BindVertexBuffer(vertexBuffer);
		BindIndexBuffer(indexBuffer);
		GLCall(glDrawElements(GL_LINES, indexBuffer->GetCount(), GL_UNSIGNED_SHORT, nullptr));
		UnbindVertexBuffer(vertexBuffer);
		UnbindIndexBuffer(indexBuffer);
		GLCall(glPolygonMode(GL_FRONT_AND_BACK, GL_FILL));
	}
}

void Renderer_OpenGL::DrawDebugCircle(const VertexBuffer* vertexBuffer, const Shader* shader)
{
	BindShader(shader);
	BindVertexBuffer(vertexBuffer);
	GLCall(glDrawArrays(GL_LINE_LOOP, 0, 16));
	UnbindVertexBuffer(vertexBuffer);
}

void Renderer_OpenGL::DrawDebugLine(const VertexBuffer* vertexBuffer, const Shader* shader)
{
	BindShader(shader);
	BindVertexBuffer(vertexBuffer);
	GLCall(glDrawArrays(GL_LINE_LOOP, 0, 2));
	UnbindVertexBuffer(vertexBuffer);
}

void Renderer_OpenGL::SetDepthTest(bool bEnabled)
{
	if (bEnabled)
	{
		glEnable(GL_DEPTH_TEST);
	}
	else
	{
		glDisable(GL_DEPTH_TEST);
	}
}

void Renderer_OpenGL::SetBlending(bool bEnabled)
{
	if (bEnabled)
	{
		glEnable(GL_BLEND);
	}
	else
	{
		glDisable(GL_BLEND);
	}
}

void Renderer_OpenGL::SetCullingFace(CullFace Face)
{
	CullingFace = Face;
	if (CullingFace == CullFace::None)
	{
		glDisable(GL_CULL_FACE);
	}
	else
	{
		GLenum GlFace = GL_NONE;

		switch (CullingFace)
		{
			case CullFace::None:
				GlFace = GL_NONE;
				break;
			case CullFace::Front:
				GlFace = GL_FRONT;
				break;
			case CullFace::Back:
				GlFace = GL_BACK;
				break;
			case CullFace::Both:
				GlFace = GL_FRONT_AND_BACK;
				break;
		}

		glEnable(GL_CULL_FACE);
		glCullFace(GlFace);
	}
}

CullFace Renderer_OpenGL::GetCullingFace()
{
	return CullingFace;
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
		BindIndexBuffer(data.mEBO);
		GLCall(glDrawElements(data.mDrawCommand, data.mEBO->GetCount(), GL_UNSIGNED_SHORT, nullptr));
		UnbindVertexArray(data.mVAO);
		UnbindIndexBuffer(data.mEBO);
	}
	mDebugRenderData.clear();
	glLineWidth(width);
	UnbindShader(shader);
}*/

const FrameBuffer* Renderer_OpenGL::GetBackBuffer()
{
	return BackBuffer;
}

void Renderer_OpenGL::SetViewportSize(glm::vec2 Offset, glm::vec2 Size)
{
	glViewport(Offset.x, Offset.y, Size.x, Size.y);
}
