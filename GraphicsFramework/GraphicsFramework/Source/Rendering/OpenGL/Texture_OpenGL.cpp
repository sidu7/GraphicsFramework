/* Start Header -------------------------------------------------------
Copyright (C) 2018-2019 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the prior
written consent of DigiPen Institute of Technology is prohibited.
Author: Sidhant Tumma
- End Header --------------------------------------------------------*/

#include "Texture_OpenGL.h"

#include "Core/Core.h"
#include "Rendering/OpenGL/Renderer_OpenGL.h"

Texture_OpenGL::Texture_OpenGL() : 
	mRendererID(0),
	mLocalBuffer(nullptr), 
	mWidth(0), 
	mHeight(0)
{
}

void Texture_OpenGL::Init(const std::string& FilePath) 
{
	mFilePath = FilePath;

	stbi_set_flip_vertically_on_load(1);
		
	// Load hdr texture
	int NumChannels;
	if (FilePath.substr(FilePath.find_last_of(".")) == ".hdr")
	{
		float* data = stbi_loadf(mFilePath.c_str(), &mWidth, &mHeight, &NumChannels, 0);

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

		mFormat = GetImageFormat<float>(NumChannels);

		if (data)
		{
			stbi_image_free(data);
		}
		return;
	}
	
	mLocalBuffer = stbi_load(mFilePath.c_str(), &mWidth, &mHeight, &NumChannels, 0);
	
	mFormat = GetImageFormat<uint8_t>(NumChannels);

	ReadBufferToTexture();

	if (mLocalBuffer)
		stbi_image_free(mLocalBuffer);
}

void Texture_OpenGL::Init(ImageFormat format, int width, int height)
{	
	mFormat = format;
	mWidth = width;
	mHeight = height;

	GLCall(glGenTextures(1, &mRendererID));
	GLCall(glBindTexture(GL_TEXTURE_2D, mRendererID));

	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
	GLCall(glGenerateMipmap(GL_TEXTURE_2D));

	GLCall(glTexImage2D(GL_TEXTURE_2D, 0, GetGlInternalFormat(), mWidth, mHeight, 0, GetGlFormat(), GL_FLOAT, NULL));
	GLCall(glBindTexture(GL_TEXTURE_2D, 0));
}

void Texture_OpenGL::Init(void* buffer, int size)
{
	int NumChannels;
	mLocalBuffer = stbi_load_from_memory(reinterpret_cast<unsigned char*>(buffer), size, &mWidth, &mHeight, &NumChannels, 0);
	
	mFormat = GetImageFormat<uint8_t>(NumChannels);
	
	ReadBufferToTexture();
}

Texture_OpenGL::~Texture_OpenGL()
{
	GLCall(glDeleteTextures(1, &mRendererID));
}

void Texture_OpenGL::EnableTiling() const
{
	GLCall(glBindTexture(GL_TEXTURE_2D, mRendererID));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT));
}

GLenum Texture_OpenGL::GetGlFormat()
{
	switch (mFormat) {
		case ImageFormat::R8:
		case ImageFormat::R16F:
		case ImageFormat::R32F:
			return GL_RED;
		case ImageFormat::RG8:
		case ImageFormat::RG16F:
		case ImageFormat::RG32F:
			return GL_RG;
		case ImageFormat::RGB8:
		case ImageFormat::RGB16F:
		case ImageFormat::RGB32F:
			return GL_RGB;
		case ImageFormat::RGBA8:
		case ImageFormat::RGBA16F:
		case ImageFormat::RGBA32F:
			return GL_RGBA;
		default:
			return GL_RGBA;
	}
}

GLenum Texture_OpenGL::GetGlInternalFormat()
{
	switch (mFormat) {
		case ImageFormat::R8:
			return GL_R8;
		case ImageFormat::RG8:
			return GL_RG8;
		case ImageFormat::RGB8:
			return GL_RGB8;
		case ImageFormat::RGBA8:
			return GL_RGBA8;
		case ImageFormat::R16F:
			return GL_R16F;
		case ImageFormat::RG16F:
			return GL_RG16F;
		case ImageFormat::RGB16F:
			return GL_RGB16F;
		case ImageFormat::RGBA16F:
			return GL_RGBA16F;
		case ImageFormat::R32F:
			return GL_R32F;
		case ImageFormat::RG32F:
			return GL_RG32F;
		case ImageFormat::RGB32F:
			return GL_RGB32F;
		case ImageFormat::RGBA32F:
			return GL_RGBA32F;
		default:
			return GL_RGBA32F;
	}
}

void Texture_OpenGL::ReadBufferToTexture()
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

	GLCall(glTexImage2D(GL_TEXTURE_2D, 0, GetGlInternalFormat(), mWidth, mHeight, 0, GetGlFormat(), GL_UNSIGNED_BYTE, mLocalBuffer));
	GLCall(glBindTexture(GL_TEXTURE_2D, 0));
}

