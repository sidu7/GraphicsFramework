#include "UniformBuffer.h"
#include "Renderer.h"

UniformBuffer::UniformBuffer(unsigned int size, const void* data)
{
	GLCall(glGenBuffers(1, &m_RendererId));

	GLCall(glBindBuffer(GL_UNIFORM_BUFFER, m_RendererId));
	GLCall(glBufferData(GL_UNIFORM_BUFFER, size, data? data : NULL, GL_STATIC_DRAW));
	GLCall(glBindBuffer(GL_UNIFORM_BUFFER, 0));
}

UniformBuffer::~UniformBuffer()
{
	GLCall(glDeleteBuffers(1, &m_RendererId));
}

void UniformBuffer::Bind(unsigned int bindPoint) const
{
	GLCall(glBindBufferBase(GL_UNIFORM_BUFFER, bindPoint, m_RendererId));
}

void UniformBuffer::Unbind() const
{
	GLCall(glBindBuffer(GL_UNIFORM_BUFFER, 0));
}

void UniformBuffer::SubData(unsigned int size, const void* data) const
{
	GLCall(glBindBuffer(GL_UNIFORM_BUFFER, m_RendererId));
	GLCall(glBufferSubData(GL_UNIFORM_BUFFER, 0, size, data));
	GLCall(glBindBuffer(GL_UNIFORM_BUFFER, 0));
}
