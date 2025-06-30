/* Start Header -------------------------------------------------------
Copyright (C) 2018-2019 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the prior
written consent of DigiPen Institute of Technology is prohibited.
Author: Sidhant Tumma
- End Header --------------------------------------------------------*/

#include "Texture_Vulkan.h"

#include "Core/Core.h"
#include "Rendering/Vulkan/Renderer_Vulkan.h"

VkFormat Texture_Vulkan::GetVkFormat(ImageFormat format)
{
	switch (format) {
		case ImageFormat::R16F:
			return VK_FORMAT_R16_SFLOAT;
		case ImageFormat::R32F:
			return VK_FORMAT_R32_SFLOAT;
		case ImageFormat::RG16F:
			return VK_FORMAT_R16G16_SFLOAT;
		case ImageFormat::RG32F:
			return VK_FORMAT_R32G32_SFLOAT;
		case ImageFormat::RGB16F:
			return VK_FORMAT_R16G16B16_SFLOAT;
		case ImageFormat::RGB32F:
			return VK_FORMAT_R32G32B32_SFLOAT;
		case ImageFormat::RGBA16F:
			return VK_FORMAT_R16G16B16A16_SFLOAT;
		case ImageFormat::RGBA32F:
			return VK_FORMAT_R32G32B32A32_SFLOAT;
	}
	}

Texture_Vulkan::Texture_Vulkan() : 
	mRendererID(0),
	mLocalBuffer(nullptr), 
	mWidth(0), 
	mHeight(0)
{
}

void Texture_Vulkan::Init(const std::string& FilePath) 
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

		if (data)
		{
			stbi_image_free(data);
		}
		return;
	}
	
	mLocalBuffer = stbi_load(mFilePath.c_str(), &mWidth, &mHeight, &NumChannels, 0);

	mFormat = GetImageFormat(NumChannels);

	ReadBufferToTexture();

	if (mLocalBuffer)
		stbi_image_free(mLocalBuffer);
}

void Texture_Vulkan::Init(ImageFormat format, int width, int height)
{	
	mFormat = format;
	mWidth = width;
	mHeight = height;
}

void Texture_Vulkan::Init(void* buffer, int size)
{
	int NumChannels;
	mLocalBuffer = stbi_load_from_memory(reinterpret_cast<unsigned char*>(buffer), size, &mWidth, &mHeight, &NumChannels, 0);

	mFormat = GetImageFormat(NumChannels);

	ReadBufferToTexture();
}

Texture_Vulkan::~Texture_Vulkan()
{
}

void Texture_Vulkan::EnableTiling() const
{
}

void Texture_Vulkan::ReadBufferToTexture()
{
	if (!mLocalBuffer)
	{
		std::cout << "[Texture] Texture " << mFilePath << " count not be loaded" << std::endl;
	}
}

