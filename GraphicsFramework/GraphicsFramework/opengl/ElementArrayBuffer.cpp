#include "ElementArrayBuffer.h"

ElementArrayBuffer::ElementArrayBuffer()
{
	GLCall(glGenBuffers(1, &m_RendererID));
}

ElementArrayBuffer::~ElementArrayBuffer()
{
	GLCall(glDeleteBuffers(1, &m_RendererID));
}

void ElementArrayBuffer::AddData(const void* data, unsigned int count, unsigned int size)
{
	Bind();
	this->count = count;
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, size * count, data, GL_STATIC_DRAW);
}

void ElementArrayBuffer::Bind() const
{
	GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_RendererID));
}

void ElementArrayBuffer::Unbind() const
{
	GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
}
