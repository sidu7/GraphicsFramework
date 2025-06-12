/* Start Header -------------------------------------------------------
Copyright (C) 2018-2019 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the prior
written consent of DigiPen Institute of Technology is prohibited.
Author: Sidhant Tumma
- End Header --------------------------------------------------------*/


#pragma once

class VertexBuffer
{
public:
	virtual ~VertexBuffer() {}
	 
	virtual void AddData(const void* data,unsigned int size) const = 0;
	virtual void AddSubData(const void* data, unsigned int size, unsigned offset = 0) const = 0;
	virtual void AddDynamicData(const void* data, unsigned int size) const = 0;
};
