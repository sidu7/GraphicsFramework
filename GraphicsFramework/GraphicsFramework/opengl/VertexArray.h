/* Start Header -------------------------------------------------------
Copyright (C) 2018-2019 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the prior
written consent of DigiPen Institute of Technology is prohibited.
Author: Sidhant Tumma
- End Header --------------------------------------------------------*/

#pragma once
#include <list>

class VertexBuffer;

class VertexElements
{
public:
	unsigned int m_Count;
	unsigned int m_Type;
	unsigned int m_Size;

public:
	VertexElements(unsigned int count, unsigned int type, unsigned int size);

};

class VertexArray
{
private:
	unsigned int m_RendererID;
	unsigned int m_Stride;
	unsigned int m_Offset;
	unsigned int m_Index;
	std::list<VertexElements> m_Elements;

public:
	VertexArray();
	~VertexArray();
	 
	void AddLayout();
	void AddBuffer(const VertexBuffer& vb) const;
	void Push(unsigned int count, unsigned int type, unsigned int size);
	void AddLayoutAtLocation(unsigned location, unsigned int type, unsigned int each_size, unsigned count, unsigned offset = 0);
	void Bind() const;
	void Unbind() const;
	void Delete() const;
};