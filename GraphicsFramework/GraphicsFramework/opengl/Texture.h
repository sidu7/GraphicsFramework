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
	int mWidth, mHeight, mBPP;
	unsigned char* mLocalBuffer;

public:
	std::string mFilePath;
	Texture(const std::string& FilePath);
	~Texture();

	void Bind(unsigned int slot = 0) const;
	void Unbind() const;
	void EnableTiling() const;
};