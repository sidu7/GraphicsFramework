/* Start Header -------------------------------------------------------
Copyright (C) 2018-2019 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the prior
written consent of DigiPen Institute of Technology is prohibited.
Author: Sidhant Tumma
- End Header --------------------------------------------------------*/

#pragma once
#include "Texture.h"

class FrameBuffer
{
public:
	virtual ~FrameBuffer() {}
	 
	virtual void Init(int width, int height, ImageFormat format, int Texcount = 1) = 0;

	virtual void CopyDepthTo(const FrameBuffer* target) = 0;
	virtual void Clear() const = 0;
	virtual void DepthBind() = 0;
	virtual void DepthUnbind() = 0;
	virtual void Delete() const = 0;
	virtual void ChangeSize(unsigned int width, unsigned int height) = 0;
	virtual const Texture* GetTexture(unsigned int index = 0) = 0;

	virtual inline int GetWidth() const = 0;
	virtual inline int GetHeight() const = 0;
};
