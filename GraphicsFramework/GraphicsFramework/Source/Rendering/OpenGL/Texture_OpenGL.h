/* Start Header -------------------------------------------------------
Copyright (C) 2018-2019 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the prior
written consent of DigiPen Institute of Technology is prohibited.
Author: Sidhant Tumma
- End Header --------------------------------------------------------*/

#pragma once
#include "Rendering/Texture.h"

class Texture_OpenGL : public Texture
{
public:
	Texture_OpenGL();
	virtual ~Texture_OpenGL();

	virtual void Init(int channels, int width, int height) override;
	virtual void Init(const std::string& FilePath) override;
	virtual void Init(void* buffer, int size) override;

	virtual void EnableTiling() const override;
	virtual inline std::string GetSourcePath() const override { return mFilePath; }

	int mWidth, mHeight, mBPP;
	std::string mFilePath;
	unsigned int mRendererID;

protected:
	void ReadBufferToTexture();

	unsigned char* mLocalBuffer;
};