#include "UniformBuffer_OpenGL.h"

#include "Rendering/Renderer.h"

UniformBuffer_OpenGL::UniformBuffer_OpenGL() :
	m_RendererId(0)
{
	GLCall(glGenBuffers(1, &m_RendererId));
}

UniformBuffer_OpenGL::~UniformBuffer_OpenGL()
{
	GLCall(glDeleteBuffers(1, &m_RendererId));
}

void UniformBuffer_OpenGL::Init(unsigned int size, const void* data)
{
	GLCall(glBindBuffer(GL_UNIFORM_BUFFER, m_RendererId));
	GLCall(glBufferData(GL_UNIFORM_BUFFER, size, data? data : NULL, GL_STATIC_DRAW));
	GLCall(glBindBuffer(GL_UNIFORM_BUFFER, 0));
}

void UniformBuffer_OpenGL::AddData(unsigned int size, const void* data, unsigned int offset) const
{
	GLCall(glBindBuffer(GL_UNIFORM_BUFFER, m_RendererId));
	GLCall(glBufferSubData(GL_UNIFORM_BUFFER, offset, size, data));
	GLCall(glBindBuffer(GL_UNIFORM_BUFFER, 0));
}
