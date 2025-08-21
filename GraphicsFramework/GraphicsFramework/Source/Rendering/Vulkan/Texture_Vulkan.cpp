/* Start Header -------------------------------------------------------
Copyright (C) 2018-2019 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the prior
written consent of DigiPen Institute of Technology is prohibited.
Author: Sidhant Tumma
- End Header --------------------------------------------------------*/

#include "Texture_Vulkan.h"

#include "Core/Core.h"
#include "Rendering/Vulkan/Renderer_Vulkan.h"
#include "Rendering/Vulkan/Internal/VulkanHelper.h"

VkFormat Texture_Vulkan::GetVkFormat(ImageFormat format)
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

	mFormat = GetImageFormat<uint8_t>(NumChannels);

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

	mFormat = GetImageFormat<uint8_t>(NumChannels);

	ReadBufferToTexture();

	if (mLocalBuffer)
		stbi_image_free(mLocalBuffer);
}

Texture_Vulkan::~Texture_Vulkan()
{
	vkUnmapMemory(Renderer_Vulkan::Get()->GetDevice(), StagingImageBuffer.BufferMemory);
	vkFreeMemory(Renderer_Vulkan::Get()->GetDevice(), StagingImageBuffer.BufferMemory, nullptr);
	vkDestroyBuffer(Renderer_Vulkan::Get()->GetDevice(), StagingImageBuffer.Buffer, nullptr);

	vkFreeMemory(Renderer_Vulkan::Get()->GetDevice(), TexImageInfo.ImageMemory, nullptr);
	vkDestroyImage(Renderer_Vulkan::Get()->GetDevice(), TexImageInfo.Image, nullptr);
	vkDestroyImageView(Renderer_Vulkan::Get()->GetDevice(), TexImageView, nullptr);
	vkDestroySampler(Renderer_Vulkan::Get()->GetDevice(), TexSampler, nullptr);
}
 
void Texture_Vulkan::EnableTiling() const
{
}

void Texture_Vulkan::ReadBufferToTexture()
{
	if (!mLocalBuffer)
	{
		std::cout << "[Texture] Texture " << mFilePath << " count not be loaded" << std::endl;
		return;
	}

	uint32_t ImageSize = mWidth * mHeight * GetNumChannels();
	RenderingFactory_Vulkan* VulkanFactory = static_cast<RenderingFactory_Vulkan*>(RenderingFactory::Instance());
	StagingImageBuffer = VulkanFactory->CreateBuffer(ImageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	vkMapMemory(Renderer_Vulkan::Get()->GetDevice(), StagingImageBuffer.BufferMemory, 0, ImageSize, 0, &StagingImageBuffer.MappedMemory);
	memcpy(StagingImageBuffer.MappedMemory, mLocalBuffer, static_cast<size_t>(ImageSize));
	vkUnmapMemory(Renderer_Vulkan::Get()->GetDevice(), StagingImageBuffer.BufferMemory);

	TexImageInfo = VulkanFactory->CreateImage(mWidth, mHeight, GetVkFormat(mFormat), VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

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

	VKCall(vkCreateImageView(Renderer_Vulkan::Get()->GetDevice(), &TexImageViewInfo, nullptr, &TexImageView), "Failed to create Image view.");

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

	VKCall(vkCreateSampler(Renderer_Vulkan::Get()->GetDevice(), &TexImageSamplerInfo, nullptr, &TexSampler), "Failed to create Tex image sampler.");
}

