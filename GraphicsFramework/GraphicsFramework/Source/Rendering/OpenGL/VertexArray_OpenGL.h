/* Start Header -------------------------------------------------------
Copyright (C) 2018-2019 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the prior
written consent of DigiPen Institute of Technology is prohibited.
Author: Sidhant Tumma
- End Header --------------------------------------------------------*/

#pragma once

#include "Rendering/VertexArray.h"
#include "Core/Core.h"

class VertexArray_OpenGL : public VertexArray
{
public:
	VertexArray_OpenGL();
	virtual ~VertexArray_OpenGL();
	 
	virtual void AddLayout() override;
	virtual void AddBuffer(const VertexBuffer* vb) const override;
	virtual void Push(unsigned int count, unsigned int type, unsigned int size) override;
	virtual void AddLayoutAtLocation(unsigned location, unsigned int type, unsigned int each_size, unsigned count, unsigned offset = 0) override;
	virtual void Delete() const override;

	unsigned int m_RendererID;
	unsigned int m_Stride;
	unsigned int m_Offset;
	unsigned int m_Index;
	std::list<VertexElements> m_Elements;
};