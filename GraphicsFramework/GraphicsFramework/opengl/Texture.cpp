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
		
	// Load hdr texture
	if (FilePath.substr(FilePath.find_last_of(".")) == ".hdr")
	{
		float* data = stbi_loadf(mFilePath.c_str(), &mWidth, &mHeight, &mBPP, 0);

		if (!data)
		{
			std::cout << "[Texture] HDR Texture {0} could not be loaded" << mFilePath << std::endl;
		}

		GLCall(glGenTextures(1, &mRendererID));
		GLCall(glBindTexture(GL_TEXTURE_2D, mRendererID));

		GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT));
		GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT));
		GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR));
		GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));

		GLCall(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, mWidth, mHeight, 0, GL_RGB, GL_FLOAT, data));
		GLCall(glGenerateMipmap(GL_TEXTURE_2D));

		GLCall(glBindTexture(GL_TEXTURE_2D, 0));

		if (data)
		{
			stbi_image_free(data);
		}
		return;
	}
	
	mLocalBuffer = stbi_load(mFilePath.c_str(), &mWidth, &mHeight, &mBPP, 0);

	ReadBufferToTexture();

	if (mLocalBuffer)
		stbi_image_free(mLocalBuffer);
}

Texture::Texture(int channels, int width, int height) : mBPP(channels), mRendererID(0), mWidth(width), mHeight(height), mLocalBuffer(nullptr)
{	
	GLCall(glGenTextures(1, &mRendererID));
	GLCall(glBindTexture(GL_TEXTURE_2D, mRendererID));

	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));

	GLint format;
	GLint internalformat;
	switch (mBPP) {

	case 1: format = GL_RED;
		internalformat = GL_R32F;
		break;
	case 3: format = GL_RGB;
		internalformat = GL_RGB32F;
		break;
	case 4: format = GL_RGBA;
		internalformat = GL_RGBA32F;
		break;
	}

	GLCall(glTexImage2D(GL_TEXTURE_2D, 0, internalformat, mWidth, mHeight, 0, format, GL_FLOAT, NULL));
	GLCall(glBindTexture(GL_TEXTURE_2D, 0));
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

