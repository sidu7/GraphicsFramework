/* Start Header -------------------------------------------------------
Copyright (C) 2018-2019 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the prior
written consent of DigiPen Institute of Technology is prohibited.
Author: Sidhant Tumma
- End Header --------------------------------------------------------*/

#pragma once

class VertexBuffer;

class VertexElements
{
public:
	unsigned int m_Count;
	unsigned int m_Type;
	unsigned int m_Size;

public:
	VertexElements(unsigned int count, unsigned int type, unsigned int size) :
		m_Count(count), m_Type(type), m_Size(size)
	{ }
};

class VertexArray
{
public:
	virtual ~VertexArray() {}
	 
	virtual void AddLayout() = 0;
	virtual void AddBuffer(const VertexBuffer* vb) const = 0;
	virtual void Push(unsigned int count, unsigned int type, unsigned int size) = 0;
	virtual void AddLayoutAtLocation(unsigned location, unsigned int type, unsigned int each_size, unsigned count, unsigned offset = 0) = 0;
	virtual void Delete() const = 0;
};