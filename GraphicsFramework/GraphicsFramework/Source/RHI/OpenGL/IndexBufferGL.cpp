#include "IndexBufferGL.h"

#include "RHI/OpenGL/DeviceGL.h"

namespace rhi::gl
{
IndexBufferGL::IndexBufferGL()
	: m_RendererID(0),	m_Count(0)
{
	GLCall(glGenBuffers(1, &m_RendererID));
}

IndexBufferGL::~IndexBufferGL()
{
	if (m_RendererID)
	{
		GLCall(glDeleteBuffers(1, &m_RendererID));
	}
	m_RendererID = 0;
}

void IndexBufferGL::AddData(const void* data, unsigned int count, IndexType type)
{
	Renderer::Instance()->BindIndexBuffer(this);
	m_Count = count;
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, GetIndexTypeSize(type) * count, data, GL_STATIC_DRAW);
	Renderer::Instance()->UnbindIndexBuffer(this);
}
}
