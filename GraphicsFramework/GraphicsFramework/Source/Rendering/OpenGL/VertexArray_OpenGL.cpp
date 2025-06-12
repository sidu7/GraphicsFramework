/* Start Header -------------------------------------------------------
Copyright (C) 2018-2019 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the prior
written consent of DigiPen Institute of Technology is prohibited.
Author: Sidhant Tumma
- End Header --------------------------------------------------------*/

#include "VertexArray_OpenGL.h"
#include "Rendering/VertexBuffer.h"
#include "Rendering/Renderer.h"

VertexArray_OpenGL::VertexArray_OpenGL() :
	m_RendererID(0),
	m_Stride(0),
	m_Offset(0),
	m_Index(0)
{
	GLCall(glGenVertexArrays(1, &m_RendererID));
}

VertexArray_OpenGL::~VertexArray_OpenGL()
{
	GLCall(glDeleteVertexArrays(1, &m_RendererID));
}

void VertexArray_OpenGL::AddLayout()
{
	for (auto c : m_Elements)
	{
		GLCall(glEnableVertexAttribArray(m_Index));
		if (c.m_Type == GL_INT)
		{
			GLCall(glVertexAttribIPointer(m_Index, c.m_Count, c.m_Type, m_Stride, (const void*)m_Offset));
		}
		else
		{
			GLCall(glVertexAttribPointer(m_Index, c.m_Count, c.m_Type, GL_FALSE, m_Stride, (const void*)m_Offset));
		}
		m_Offset += c.m_Size * c.m_Count;
		m_Index++;
	}
	m_Elements.clear();
	m_Offset = 0;
	m_Stride = 0;
}

void VertexArray_OpenGL::Push(unsigned int count, unsigned int type, unsigned int size)
{
	VertexElements element(count, type, size);
	m_Elements.push_back(element);
	m_Stride += count * size;
}

void VertexArray_OpenGL::AddLayoutAtLocation(unsigned location, unsigned int type, unsigned int each_size, unsigned count, unsigned offset)
{
	glVertexAttribPointer(location, count, type, GL_FALSE, each_size * count, (const void*)offset);
	glEnableVertexAttribArray(location);
}

void VertexArray_OpenGL::AddBuffer(const VertexBuffer* vb) const
{
	Renderer::Instance()->BindVertexArray(this);
	if (vb)
	{
		Renderer::Instance()->BindVertexBuffer(vb);
	}
}

void VertexArray_OpenGL::Delete() const
{
	GLCall(glDeleteVertexArrays(1, &m_RendererID));
}