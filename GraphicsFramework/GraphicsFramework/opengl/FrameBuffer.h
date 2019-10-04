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
private:
	unsigned int m_RendererID;

public:
	unsigned int *m_TextureID;
	unsigned int mWidth, mHeight, mDepthBuffer;
	unsigned int mTexCount;
public:
	FrameBuffer(int width, int height, int Texcount = 1);
	~FrameBuffer();

	void Bind() const;
	void Unbind() const;
	void TexBind(unsigned int index = 0, unsigned int slot = 0);
	void TexUnbind(unsigned int index = 0);
	void Clear() const;
	void DepthBind();
	void DepthUnbind();
	void Delete() const;
	void ChangeSize(unsigned int width, unsigned int height);
};
