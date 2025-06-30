/* Start Header -------------------------------------------------------
Copyright (C) 2018-2019 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the prior
written consent of DigiPen Institute of Technology is prohibited.
Author: Sidhant Tumma
- End Header --------------------------------------------------------*/

#include "VertexBuffer_OpenGL.h"
#include "Rendering/OpenGL/Renderer_OpenGL.h"


VertexBuffer_OpenGL::VertexBuffer_OpenGL()
{
	GLCall(glGenVertexArrays(1, &m_VertexArrayID));
	GLCall(glGenBuffers(1, &m_VertexBufferID));	
}

VertexBuffer_OpenGL::~VertexBuffer_OpenGL()
{
	Delete();
}

void VertexBuffer_OpenGL::AddData(const void* data, unsigned int size)
{
	Renderer::Instance()->BindVertexBuffer(this);
	GLCall(glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW));
	Renderer::Instance()->UnbindVertexBuffer(this);
}

void VertexBuffer_OpenGL::AddSubData(const void* data, unsigned int size, unsigned offset) 
{
	Renderer::Instance()->BindVertexBuffer(this);
	GLCall(glBufferSubData(GL_ARRAY_BUFFER, offset, size, data));
	Renderer::Instance()->UnbindVertexBuffer(this);
}

void VertexBuffer_OpenGL::AddDynamicData(const void* data, unsigned int size)
{
	Renderer::Instance()->BindVertexBuffer(this);
	GLCall(glBufferData(GL_ARRAY_BUFFER, size, data, GL_DYNAMIC_DRAW));
	Renderer::Instance()->UnbindVertexBuffer(this);
}

void VertexBuffer_OpenGL::SetDataLayout(const std::vector<VertexFormat>& vertexFormats)
{
	Renderer::Instance()->BindVertexBuffer(this);

	uint16_t Offset = 0;

	for (VertexFormat format : vertexFormats)
	{
		mStride += GetFormatSize(format);
	}

	mVertexLayouts.resize(vertexFormats.size());
	for (uint16_t i = 0; i < vertexFormats.size(); ++i)
	{
		uint16_t Location = i;
		switch (vertexFormats[i])
		{
			case VertexFormat::Bool:
				GLCall(glVertexAttribPointer(Location, 1, GL_BOOL, GL_FALSE, mStride, reinterpret_cast<const void*>(Offset)));
				break;
			case VertexFormat::UInt:
				GLCall(glVertexAttribIPointer(Location, 1, GL_UNSIGNED_INT, mStride, reinterpret_cast<const void*>(Offset)));
				break;
			case VertexFormat::Int:
				GLCall(glVertexAttribIPointer(Location, 1, GL_INT, mStride, reinterpret_cast<const void*>(Offset)));
				break;
			case VertexFormat::iVec2:
				GLCall(glVertexAttribIPointer(Location, 2, GL_INT, mStride, reinterpret_cast<const void*>(Offset)));
				break;
			case VertexFormat::iVec3:
				GLCall(glVertexAttribIPointer(Location, 3, GL_INT, mStride, reinterpret_cast<const void*>(Offset)));
				break;
			case VertexFormat::iVec4:
				GLCall(glVertexAttribIPointer(Location, 4, GL_INT, mStride, reinterpret_cast<const void*>(Offset)));
				break;
			case VertexFormat::Float:
				GLCall(glVertexAttribPointer(Location, 1, GL_FLOAT, GL_FALSE, mStride, reinterpret_cast<const void*>(Offset)));
				break;
			case VertexFormat::Vec2:
				GLCall(glVertexAttribPointer(Location, 2, GL_FLOAT, GL_FALSE, mStride, reinterpret_cast<const void*>(Offset)));
				break;
			case VertexFormat::Vec3:
				GLCall(glVertexAttribPointer(Location, 3, GL_FLOAT, GL_FALSE, mStride, reinterpret_cast<const void*>(Offset)));
				break;
			case VertexFormat::Vec4:
				GLCall(glVertexAttribPointer(Location, 4, GL_FLOAT, GL_FALSE, mStride, reinterpret_cast<const void*>(Offset)));
				break;
			case VertexFormat::Mat2:
				GLCall(glVertexAttribPointer(Location, 4, GL_FLOAT, GL_FALSE, mStride, reinterpret_cast<const void*>(Offset)));
				break;
			case VertexFormat::Mat3:
				GLCall(glVertexAttribPointer(Location, 9, GL_FLOAT, GL_FALSE, mStride, reinterpret_cast<const void*>(Offset)));
				break;
			case VertexFormat::Mat4:
				GLCall(glVertexAttribPointer(Location, 16, GL_FLOAT, GL_FALSE, mStride, reinterpret_cast<const void*>(Offset)));
				break;
			default:
				break;
		}

		mVertexLayouts[i].mOffset = Offset;
		mVertexLayouts[i].mFormat = vertexFormats[i];

		glEnableVertexAttribArray(Location);
		Offset += GetFormatSize(vertexFormats[i]);
	}
	Renderer::Instance()->UnbindVertexBuffer(this);
}

void VertexBuffer_OpenGL::Delete() const
{
	GLCall(glDeleteVertexArrays(1, &m_VertexArrayID));
	GLCall(glDeleteBuffers(1, &m_VertexBufferID));
}