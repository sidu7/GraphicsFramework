/* Start Header -------------------------------------------------------
Copyright (C) 2018-2019 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the prior
written consent of DigiPen Institute of Technology is prohibited.
Author: Sidhant Tumma
- End Header --------------------------------------------------------*/

#include "VertexBuffer_OpenGL.h"
#include "Rendering/Renderer.h"


VertexBuffer_OpenGL::VertexBuffer_OpenGL()
{
	GLCall(glGenBuffers(1, &m_RendererID));	
}

VertexBuffer_OpenGL::~VertexBuffer_OpenGL()
{
	GLCall(glDeleteBuffers(1, &m_RendererID));
}

void VertexBuffer_OpenGL::AddData(const void* data, unsigned int size) const
{
	Renderer::Instance()->BindVertexBuffer(this);
	GLCall(glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW));
}

void VertexBuffer_OpenGL::AddSubData(const void* data, unsigned int size, unsigned offset) const
{
	Renderer::Instance()->BindVertexBuffer(this);
	GLCall(glBufferSubData(GL_ARRAY_BUFFER, offset, size, data));
}

void VertexBuffer_OpenGL::AddDynamicData(const void* data, unsigned int size) const
{
	Renderer::Instance()->BindVertexBuffer(this);
	GLCall(glBufferData(GL_ARRAY_BUFFER, size, data, GL_DYNAMIC_DRAW));
}
