/* Start Header -------------------------------------------------------
Copyright (C) 2018-2019 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the prior
written consent of DigiPen Institute of Technology is prohibited.
Author: Sidhant Tumma
- End Header --------------------------------------------------------*/

#include "TextureVK.h"

#include "Core/Core.h"
#include "RHI/Vulkan/DeviceVK.h"
#include "RHI/Vulkan/Internal/HelperVK.h"

namespace rhi::vk
{
VkFormat TextureVK::GetVkFormat(ImageFormat format)
{
	switch (format) {
		case ImageFormat::R8:
			return VK_FORMAT_R8_SRGB;
		case ImageFormat::R16F:
			return VK_FORMAT_R16_SFLOAT;
		case ImageFormat::R32F:
			return VK_FORMAT_R32_SFLOAT;
		case ImageFormat::RG8:
			return VK_FORMAT_R8G8_SRGB;
		case ImageFormat::RG16F:
			return VK_FORMAT_R16G16_SFLOAT;
		case ImageFormat::RG32F:
			return VK_FORMAT_R32G32_SFLOAT;
		case ImageFormat::RGB8:
			return VK_FORMAT_R8G8B8_SRGB;
		case ImageFormat::RGB16F:
			return VK_FORMAT_R16G16B16_SFLOAT;
		case ImageFormat::RGB32F:
			return VK_FORMAT_R32G32B32_SFLOAT;
		case ImageFormat::RGBA8:
			return VK_FORMAT_R8G8B8A8_SRGB;
		case ImageFormat::RGBA16F:
			return VK_FORMAT_R16G16B16A16_SFLOAT;
		case ImageFormat::RGBA32F:
			return VK_FORMAT_R32G32B32A32_SFLOAT;
	}

	return VK_FORMAT_UNDEFINED;
}

TextureVK::TextureVK() : 
	mRendererID(0),
	mLocalBuffer(nullptr), 
	mWidth(0), 
	mHeight(0),
	TexImageView(VK_NULL_HANDLE),
	TexSampler(VK_NULL_HANDLE)
{
}

void TextureVK::Init(const std::string& FilePath) 
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

	mFormat = GetImageFormat<uint8_t>(NumChannels);

	ReadBufferToTexture();

	if (mLocalBuffer)
		stbi_image_free(mLocalBuffer);
}

void TextureVK::Init(ImageFormat format, int width, int height)
{	
	mFormat = format;
	mWidth = width;
	mHeight = height;
}

void TextureVK::Init(void* buffer, int size)
{
	int NumChannels;
	mLocalBuffer = stbi_load_from_memory(reinterpret_cast<unsigned char*>(buffer), size, &mWidth, &mHeight, &NumChannels, 0);

	mFormat = GetImageFormat<uint8_t>(NumChannels);

	ReadBufferToTexture();

	if (mLocalBuffer)
		stbi_image_free(mLocalBuffer);
}

TextureVK::~TextureVK()
{
	Device* VkDev = Device::Get();

	VkDev->DestroyBuffer(StagingImageBuffer);
	VkDev->DestroyImage(TexImageInfo);
	vkDestroyImageView(VkDev->GetDevice(), TexImageView, nullptr);
	vkDestroySampler(VkDev->GetDevice(), TexSampler, nullptr);
}
 
void TextureVK::EnableTiling() const
{
}

void TextureVK::ReadBufferToTexture()
{
	if (!mLocalBuffer)
	{
		std::cout << "[Texture] Texture " << mFilePath << " count not be loaded" << std::endl;
		return;
	}

	uint32_t ImageSize = mWidth * mHeight * GetNumChannels();
	StagingImageBuffer = Device::Get()->CreateBuffer(ImageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, true /*bHostVisible*/);

	void* MappedData = Device::Get()->MapBuffer(StagingImageBuffer);
	memcpy(MappedData, mLocalBuffer, static_cast<size_t>(ImageSize));
	Device::Get()->UnmapBuffer(StagingImageBuffer);

	TexImageInfo = Device::Get()->CreateImage(mWidth, mHeight, GetVkFormat(mFormat), VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);

	// Texture Image View
	VkImageViewCreateInfo TexImageViewInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
	TexImageViewInfo.image = TexImageInfo.Image;
	TexImageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	TexImageViewInfo.format = GetVkFormat(mFormat);
	TexImageViewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	TexImageViewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	TexImageViewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	TexImageViewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
	TexImageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	TexImageViewInfo.subresourceRange.baseArrayLayer = 0;
	TexImageViewInfo.subresourceRange.baseMipLevel = 0;
	TexImageViewInfo.subresourceRange.layerCount = 1;
	TexImageViewInfo.subresourceRange.levelCount = 1;

	VKCall(vkCreateImageView(Device::Get()->GetDevice(), &TexImageViewInfo, nullptr, &TexImageView), "Failed to create Image view.");

	// Texture Sampler
	VkSamplerCreateInfo TexImageSamplerInfo = { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
	TexImageSamplerInfo.magFilter = VK_FILTER_LINEAR;
	TexImageSamplerInfo.minFilter = VK_FILTER_LINEAR;
	TexImageSamplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	TexImageSamplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	TexImageSamplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	TexImageSamplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	TexImageSamplerInfo.mipLodBias = 0.f;
	TexImageSamplerInfo.anisotropyEnable = VK_TRUE;
	TexImageSamplerInfo.maxAnisotropy = 4;
	TexImageSamplerInfo.compareEnable = VK_FALSE;
	TexImageSamplerInfo.compareOp = VK_COMPARE_OP_LESS;
	TexImageSamplerInfo.minLod = 0.f;
	TexImageSamplerInfo.maxLod = 0.f;
	TexImageSamplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	TexImageSamplerInfo.unnormalizedCoordinates = VK_FALSE;

	VKCall(vkCreateSampler(Device::Get()->GetDevice(), &TexImageSamplerInfo, nullptr, &TexSampler), "Failed to create Tex image sampler.");
}

}
