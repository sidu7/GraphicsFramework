/* Start Header -------------------------------------------------------
Copyright (C) 2018-2019 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the prior
written consent of DigiPen Institute of Technology is prohibited.
Author: Sidhant Tumma
- End Header --------------------------------------------------------*/

#pragma once
#include "DeviceGL.h"
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
// Rendering OpenGL
#include "RHI/OpenGL/FrameBufferGL.h"
#include "RHI/OpenGL/IndexBufferGL.h"
#include "RHI/OpenGL/VertexBufferGL.h"
#include "RHI/OpenGL/UniformBufferGL.h"
#include "RHI/OpenGL/ShaderStorageBufferGL.h"
#include "RHI/OpenGL/ShaderGL.h"
#include "RHI/OpenGL/ComputeShaderGL.h"
#include "RHI/OpenGL/TextureGL.h"
#include "Profiling/Profiler.h"
#include "Core/Window.h"
#include "Imgui/backends/imgui_impl_sdl2.h"
#include "Imgui/backends/imgui_impl_opengl3.h"
#include "RHI/OpenGL/WindowGL.h"

class VertexArray;
class VertexBuffer;
class IndexBuffer;

namespace rhi::gl
{
ComputeShader* Device::CreateComputeShader()
{
	return new ComputeShaderGL();
}

IndexBuffer* Device::CreateIndexBuffer()
{
	return new IndexBufferGL();
}

FrameBuffer* Device::CreateFrameBuffer()
{
	return new FrameBufferGL();
}

Shader* Device::CreateShader()
{
	return new ShaderGL();
}

ShaderStorageBuffer* Device::CreateShaderStorageBuffer()
{
	return new ShaderStorageBufferGL();
}

Texture* Device::CreateTexture()
{
	return new TextureGL();
}

UniformBuffer* Device::CreateUniformBuffer()
{
	return new UniformBufferGL();
}

VertexBuffer* Device::CreateVertexBuffer()
{
	return new VertexBufferGL();
}

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

void Device::Init()
{
	//Enable experimental mode for core profile contexts (required for proper
	//loading of extension function pointers such as glGenQueriesEXT).
	glewExperimental = GL_TRUE;

	//Initialize GLEW
	if (glewInit() != GLEW_OK)
	{
		printf("Error initializing GLEW\n");
	}

	GLCall(glEnable(GL_DEPTH_TEST));
	GLCall(glBlendFunc(GL_ONE, GL_ONE));

	BackBuffer = CreateFrameBuffer();
	FrameBufferGL* BackBufferOpenGl = static_cast<FrameBufferGL*>(BackBuffer);
	BackBufferOpenGl->mWidth = Window::Instance()->GetWidth();
	BackBufferOpenGl->mHeight = Window::Instance()->GetHeight();

	CullingFace = CullFace::Back;
}

void Device::Close()
{
	delete BackBuffer;
}

void Device::Clear() const
{
	glClearColor(mClearColor.x, mClearColor.y, mClearColor.z, 1.0f);
	GLCall(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
}

void Device::StartFrame()
{
	Clear();
}

void Device::SwapBuffers()
{
	SDL_GL_SwapWindow(Window::Instance()->GetWindow());
}

void Device::BindShader(const Shader* shader)
{
	if (shader)
	{
		GLCall(glUseProgram(static_cast<const ShaderGL*>(shader)->m_RendererID));
	}
}

void Device::BindComputeShader(const ComputeShader* shader)
{
	if (shader)
	{
		GLCall(glUseProgram(static_cast<const ComputeShaderGL*>(shader)->m_RendererID));
	}
}

void Device::BindFrameBuffer(const FrameBuffer* framebuffer)
{
	if (framebuffer)
	{
		const FrameBufferGL* frameBufferOpenGL = static_cast<const FrameBufferGL*>(framebuffer);
		glViewport(0, 0, frameBufferOpenGL->mWidth, frameBufferOpenGL->mHeight);
		GLCall(glBindFramebuffer(GL_FRAMEBUFFER, frameBufferOpenGL->m_RendererID));
	}
}

void Device::BindVertexBuffer(const VertexBuffer* vertexBuffer)
{
	if (vertexBuffer)
	{
		GLCall(glBindVertexArray(static_cast<const VertexBufferGL*>(vertexBuffer)->m_VertexArrayID));
		GLCall(glBindBuffer(GL_ARRAY_BUFFER, static_cast<const VertexBufferGL*>(vertexBuffer)->m_VertexBufferID));
	}
}

void Device::BindIndexBuffer(const IndexBuffer* indexBuffer)
{
	if (indexBuffer)
	{
		GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, static_cast<const IndexBufferGL*>(indexBuffer)->m_RendererID));
	}
}

void Device::BindUniformBuffer(const UniformBuffer* uniformBuffer, unsigned int bindpoint)
{
	if (uniformBuffer)
	{
		GLCall(glBindBufferBase(GL_UNIFORM_BUFFER, bindpoint, static_cast<const UniformBufferGL*>(uniformBuffer)->m_RendererId));
	}
}

void Device::BindTexture(const Texture* texture, unsigned int slot)
{
	if (texture)
	{
		GLCall(glActiveTexture(GL_TEXTURE0 + slot));
		GLCall(glBindTexture(GL_TEXTURE_2D, static_cast<const TextureGL*>(texture)->mRendererID));
	}
}

void Device::UnbindShader(const Shader* shader)
{
	GLCall(glUseProgram(0));
}

void Device::UnbindComputeShader(const ComputeShader* shader)
{
	GLCall(glUseProgram(0));
}

void Device::UnbindFrameBuffer(const FrameBuffer* framebuffer)
{
	GLCall(glBindFramebuffer(GL_FRAMEBUFFER, 0));
}

void Device::UnbindVertexBuffer(const VertexBuffer* vertexBuffer)
{
	GLCall(glBindVertexArray(0));
	GLCall(glBindBuffer(GL_ARRAY_BUFFER, 0));
}

void Device::UnbindIndexBuffer(const IndexBuffer* elementArrayBuffer)
{
	GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
}

void Device::UnbindUniformBuffer(const UniformBuffer* uniformBuffer)
{
	GLCall(glBindBuffer(GL_UNIFORM_BUFFER, 0));
}

void Device::UnbindTexture(const Texture* texture, unsigned int slot)
{
	GLCall(glActiveTexture(GL_TEXTURE0 + slot));
	GLCall(glBindTexture(GL_TEXTURE_2D, 0));
}

void Device::Draw(const Shader* Shader, const VertexBuffer* vertexBuffer, int32_t VertexCount, int32_t VertexStart, int32_t InstanceCount, int32_t InstanceStart)
{
	Profiler::Instance()->BeginDrawCall("", Shader ? Shader->GetName() : "", "");

	BindShader(Shader);
	VertexBuffer* Vb = vertexBuffer ? const_cast<VertexBuffer*>(vertexBuffer) : CreateVertexBuffer();
	BindVertexBuffer(Vb);
	GLCall(glDrawArrays(GL_TRIANGLES, 0, VertexCount));
	UnbindVertexBuffer(Vb);
	UnbindShader(Shader);
	if (!vertexBuffer)
	{
		delete Vb;
	}

	Profiler::Instance()->EndDrawCall();
}
void Device::Draw(const VertexBuffer* vertexBuffer, const IndexBuffer* indexBuffer, const Shader* shader)
{
	if (!indexBuffer)
		return;

	Profiler::Instance()->BeginDrawCall("", shader ? shader->GetName() : "", "");

	BindShader(shader);
	BindVertexBuffer(vertexBuffer);
	BindIndexBuffer(indexBuffer);
	GLCall(glDrawElements(GL_TRIANGLES, indexBuffer->GetCount(), GL_UNSIGNED_SHORT, nullptr));
	UnbindVertexBuffer(vertexBuffer);
	UnbindIndexBuffer(indexBuffer);

	Profiler::Instance()->EndDrawCall();
}

void Device::DebugDraw(const VertexBuffer* vertexBuffer, const IndexBuffer* indexBuffer, const Shader* shader)
{
	if (!indexBuffer)
		return;

	Profiler::Instance()->BeginDrawCall("", shader ? shader->GetName() : "", "");

	BindShader(shader);
	BindVertexBuffer(vertexBuffer);
	BindIndexBuffer(indexBuffer);
	GLCall(glDrawElements(GL_LINE_LOOP, indexBuffer->GetCount(), GL_UNSIGNED_SHORT, nullptr));
	UnbindVertexBuffer(vertexBuffer);
	UnbindIndexBuffer(indexBuffer);

	Profiler::Instance()->EndDrawCall();
}

void Device::DebugDrawLines(const VertexBuffer* vertexBuffer, const IndexBuffer* indexBuffer, const Shader* shader)
{
	if (!indexBuffer)
		return;

	Profiler::Instance()->BeginDrawCall("", shader ? shader->GetName() : "", "");

	GLCall(glPolygonMode(GL_FRONT_AND_BACK, GL_LINE));
	BindShader(shader);
	BindVertexBuffer(vertexBuffer);
	BindIndexBuffer(indexBuffer);
	GLCall(glDrawElements(GL_LINES, indexBuffer->GetCount(), GL_UNSIGNED_SHORT, nullptr));
	UnbindVertexBuffer(vertexBuffer);
	UnbindIndexBuffer(indexBuffer);
	GLCall(glPolygonMode(GL_FRONT_AND_BACK, GL_FILL));

	Profiler::Instance()->EndDrawCall();
}

void Device::DrawDebugCircle(const VertexBuffer* vertexBuffer, const Shader* shader)
{
	Profiler::Instance()->BeginDrawCall("", shader ? shader->GetName() : "", "");

	BindShader(shader);
	BindVertexBuffer(vertexBuffer);
	GLCall(glDrawArrays(GL_LINE_LOOP, 0, 16));
	UnbindVertexBuffer(vertexBuffer);

	Profiler::Instance()->EndDrawCall();
}

void Device::DrawDebugLine(const VertexBuffer* vertexBuffer, const Shader* shader)
{
	Profiler::Instance()->BeginDrawCall("", shader ? shader->GetName() : "", "");

	BindShader(shader);
	BindVertexBuffer(vertexBuffer);
	GLCall(glDrawArrays(GL_LINE_LOOP, 0, 2));
	UnbindVertexBuffer(vertexBuffer);

	Profiler::Instance()->EndDrawCall();
}

void Device::SetDepthTest(bool bEnabled)
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

void Device::SetBlending(bool bEnabled)
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

void Device::SetCullingFace(CullFace Face)
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

CullFace Device::GetCullingFace()
{
	return CullingFace;
}

// Debug rendering code
/*void Device::DrawDebugObjects(Shader* shader)
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

const FrameBuffer* Device::GetBackBuffer()
{
	return BackBuffer;
}

void Device::SetViewportSize(Rect3D viewport)
{
	ViewportSize = viewport;
	glViewport(ViewportSize.Offset.x, ViewportSize.Offset.y, ViewportSize.Size.x, ViewportSize.Size.y);
}

void Device::SetScissorSize(Rect2D scissor)
{
	ScissorSize = scissor;
	glScissor(ViewportSize.Offset.x, ViewportSize.Offset.y, ViewportSize.Size.x, ViewportSize.Size.y);
}

float Device::GetProjectionYFlip() const
{
	return 1.0f; // OpenGL clip space: Y goes up
}

void Device::InitImGui()
{
	rhi::gl::WindowGL* WindowGL = static_cast<rhi::gl::WindowGL*>(Window::Instance());
	ImGui_ImplSDL2_InitForOpenGL(WindowGL->GetWindow(), WindowGL->GetContext());
	ImGui_ImplOpenGL3_Init("#version 430");

	// Build font texture atlas eagerly so it's available for the first NewFrame() call
	ImGui_ImplOpenGL3_CreateFontsTexture();
}

void Device::NewFrame()
{
	ImGui_ImplOpenGL3_NewFrame();
}

void Device::Render()
{
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void Device::ShutdownImGui()
{
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplSDL2_Shutdown();
}
}
