/* Start Header -------------------------------------------------------
Copyright (C) 2018-2019 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the prior
written consent of DigiPen Institute of Technology is prohibited.
Author: Sidhant Tumma
- End Header --------------------------------------------------------*/

#pragma once
#include "Renderer.h"
#include <string>

class Texture
{
private:
	unsigned int mRendererID;
	unsigned char* mLocalBuffer;

public:
	int mWidth, mHeight, mBPP;

public:
	std::string mFilePath;
	Texture(int channels, int width, int height);
	Texture(const std::string& FilePath);
	Texture(void* buffer, int size);
	~Texture();

	inline unsigned int GetTextureID() { return mRendererID; }

	void Bind(unsigned int slot = 0) const;
	void Unbind(unsigned int slot = 0) const;
	void EnableTiling() const;

private:
	void ReadBufferToTexture();
};