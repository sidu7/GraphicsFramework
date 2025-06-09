/* Start Header -------------------------------------------------------
Copyright (C) 2018-2019 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the prior
written consent of DigiPen Institute of Technology is prohibited.
Author: Sidhant Tumma
- End Header --------------------------------------------------------*/


#pragma once

class VertexBuffer
{
private:
	unsigned int m_RendererID;
public:
	VertexBuffer();
	~VertexBuffer();
	 
	void AddData(const void* data,unsigned int size) const;
	void AddSubData(const void* data, unsigned int size, unsigned offset = 0) const;
	void AddDynamicData(const void* data, unsigned int size) const;
	void Bind() const;
	void Unbind() const;
};
