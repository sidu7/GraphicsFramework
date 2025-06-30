#include "IndexBuffer_OpenGL.h"

#include "Rendering/OpenGL/Renderer_OpenGL.h"

IndexBuffer_OpenGL::IndexBuffer_OpenGL()
	: m_RendererID(0),	m_Count(0)
{
	GLCall(glGenBuffers(1, &m_RendererID));
}

IndexBuffer_OpenGL::~IndexBuffer_OpenGL()
{
	GLCall(glDeleteBuffers(1, &m_RendererID));
}

void IndexBuffer_OpenGL::AddData(const void* data, unsigned int count, IndexType type)
{
	Renderer::Instance()->BindIndexBuffer(this);
	m_Count = count;
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, GetIndexTypeSize(type) * count, data, GL_STATIC_DRAW);
	Renderer::Instance()->UnbindIndexBuffer(this);
}
