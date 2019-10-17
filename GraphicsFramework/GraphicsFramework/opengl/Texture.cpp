/* Start Header -------------------------------------------------------
Copyright (C) 2018-2019 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the prior
written consent of DigiPen Institute of Technology is prohibited.
Author: Sidhant Tumma
- End Header --------------------------------------------------------*/

#include "Texture.h"
#include "stbi_image/stbi_image.h"
#include <iostream>


Texture::Texture(const std::string& FilePath) : mRendererID(0), mFilePath(FilePath), mWidth(0), mHeight(0), mBPP(0), mLocalBuffer(nullptr)
{
	stbi_set_flip_vertically_on_load(1);

	mLocalBuffer = stbi_load(mFilePath.c_str(), &mWidth, &mHeight, &mBPP, 0);

	ReadBufferToTexture();

	if (mLocalBuffer)
		stbi_image_free(mLocalBuffer);
}

Texture::Texture(void* buffer, int size)
{
	mLocalBuffer = stbi_load_from_memory(reinterpret_cast<unsigned char*>(buffer), size, &mWidth, &mHeight, &mBPP, 0);

	ReadBufferToTexture();
}

Texture::~Texture()
{
	GLCall(glDeleteTextures(1, &mRendererID));
}

void Texture::Bind(unsigned int slot) const
{
	GLCall(glActiveTexture(GL_TEXTURE0 + slot));
	GLCall(glBindTexture(GL_TEXTURE_2D, mRendererID));
}

void Texture::Unbind() const
{
	GLCall(glBindTexture(GL_TEXTURE_2D, 0));
}

void Texture::EnableTiling() const
{
	GLCall(glBindTexture(GL_TEXTURE_2D, mRendererID));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT));
}

void Texture::ReadBufferToTexture()
{
	if (!mLocalBuffer)
	{
		std::cout << "[Texture] Texture " << mFilePath << " count not be loaded" << std::endl;
	}

	GLCall(glGenTextures(1, &mRendererID));
	GLCall(glBindTexture(GL_TEXTURE_2D, mRendererID));

	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));

	GLint format;
	switch (mBPP) {

	case 1: format = GL_RED;
		break;
	case 3: format = GL_RGB;
		break;
	case 4: format = GL_RGBA;
		break;
	}
	GLCall(glTexImage2D(GL_TEXTURE_2D, 0, format, mWidth, mHeight, 0, format, GL_UNSIGNED_BYTE, mLocalBuffer));
	GLCall(glBindTexture(GL_TEXTURE_2D, 0));
}

