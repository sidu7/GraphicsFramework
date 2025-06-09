/* Start Header -------------------------------------------------------
Copyright (C) 2018-2019 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the prior
written consent of DigiPen Institute of Technology is prohibited.
Author: Sidhant Tumma
- End Header --------------------------------------------------------*/

#include "VertexBuffer.h"
#include "Renderer.h"


VertexBuffer::VertexBuffer()
{
	GLCall(glGenBuffers(1, &m_RendererID));	
}

VertexBuffer::~VertexBuffer()
{
	//GLCall(glDeleteBuffers(1, &m_RendererID));
}

void VertexBuffer::AddData(const void* data, unsigned int size) const
{
	Bind();
	GLCall(glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW));
}

void VertexBuffer::AddSubData(const void* data, unsigned size, unsigned offset) const
{
	Bind();
	GLCall(glBufferSubData(GL_ARRAY_BUFFER, offset, size, data));
}

void VertexBuffer::AddDynamicData(const void* data, unsigned size) const
{
	Bind();
	GLCall(glBufferData(GL_ARRAY_BUFFER, size, data, GL_DYNAMIC_DRAW));
}

void VertexBuffer::Bind() const
{
	GLCall(glBindBuffer(GL_ARRAY_BUFFER, m_RendererID));
}

void VertexBuffer::Unbind() const
{
	GLCall(glBindBuffer(GL_ARRAY_BUFFER, 0));
}
