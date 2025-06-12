#include "ElementArrayBuffer_OpenGL.h"

ElementArrayBuffer_OpenGL::ElementArrayBuffer_OpenGL() 
	: m_RendererID(0),	m_Count(0)
{
	GLCall(glGenBuffers(1, &m_RendererID));
}

ElementArrayBuffer_OpenGL::~ElementArrayBuffer_OpenGL()
{
	GLCall(glDeleteBuffers(1, &m_RendererID));
}

void ElementArrayBuffer_OpenGL::AddData(const void* data, unsigned int count, unsigned int size)
{
	Renderer::Instance()->BindElementArrayBuffer(this);
	m_Count = count;
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, size * count, data, GL_STATIC_DRAW);
}
