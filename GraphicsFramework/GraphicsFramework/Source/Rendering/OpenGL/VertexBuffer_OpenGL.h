/* Start Header -------------------------------------------------------
Copyright (C) 2018-2019 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the prior
written consent of DigiPen Institute of Technology is prohibited.
Author: Sidhant Tumma
- End Header --------------------------------------------------------*/

#pragma once

#include "Rendering/VertexBuffer.h"

class VertexBuffer_OpenGL : public VertexBuffer
{
public:
	VertexBuffer_OpenGL();
	virtual ~VertexBuffer_OpenGL();
	 
	virtual void AddData(const void* data,unsigned int size) const override;
	virtual void AddSubData(const void* data, unsigned int size, unsigned offset = 0) const override;
	virtual void AddDynamicData(const void* data, unsigned int size) const override;

	unsigned int m_RendererID;
};
