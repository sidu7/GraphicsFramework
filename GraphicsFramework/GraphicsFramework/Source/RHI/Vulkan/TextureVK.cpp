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
#include "RHI/Vulkan/Internal/CommandBufferVK.h"
#include "RHI/Vulkan/Internal/CommandQueueVK.h"
#include "Core/Profiling/Profiler.h"

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
	GF_PROFILE_SCOPE("TextureVK::Init");
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
	GF_PROFILE_SCOPE("TextureVK::Init (memory)");
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
	if (TexImageView != VK_NULL_HANDLE)
	{
		vkDestroyImageView(VkDev->GetDevice(), TexImageView, nullptr);
	}
	if (TexSampler != VK_NULL_HANDLE)
	{
		vkDestroySampler(VkDev->GetDevice(), TexSampler, nullptr);
	}
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

	const int SourceChannels = GetNumChannels();
	if (SourceChannels <= 0)
	{
		std::cout << "[Texture] Unsupported texture format for " << mFilePath << std::endl;
		return;
	}

	// Vulkan does not guarantee 3-channel image formats (and many GPUs reject
	// R8G8B8_SRGB for optimal-tiling sampled images), so every 8-bit source is
	// expanded to RGBA8 before it is uploaded.
	const bool bIs8Bit = mFormat == ImageFormat::R8 || mFormat == ImageFormat::RG8 ||
		mFormat == ImageFormat::RGB8 || mFormat == ImageFormat::RGBA8;

	std::vector<unsigned char> ExpandedPixels;
	const unsigned char* UploadPixels = mLocalBuffer;
	uint32_t UploadChannels = static_cast<uint32_t>(SourceChannels);
	VkFormat UploadFormat = GetVkFormat(mFormat);

	if (bIs8Bit && SourceChannels != 4)
	{
		const size_t PixelCount = static_cast<size_t>(mWidth) * static_cast<size_t>(mHeight);
		ExpandedPixels.resize(PixelCount * 4);

		for (size_t PixelIdx = 0; PixelIdx < PixelCount; ++PixelIdx)
		{
			const unsigned char* SourcePixel = mLocalBuffer + PixelIdx * static_cast<size_t>(SourceChannels);
			unsigned char* DestPixel = ExpandedPixels.data() + PixelIdx * 4;
			DestPixel[0] = SourcePixel[0];
			DestPixel[1] = SourceChannels > 1 ? SourcePixel[1] : 0;
			DestPixel[2] = SourceChannels > 2 ? SourcePixel[2] : 0;
			DestPixel[3] = 255;
		}

		UploadPixels = ExpandedPixels.data();
		UploadChannels = 4;
		UploadFormat = GetVkFormat(ImageFormat::RGBA8);
	}

	const VkDeviceSize ImageSize = static_cast<VkDeviceSize>(mWidth) * static_cast<VkDeviceSize>(mHeight) * UploadChannels;
	StagingImageBuffer = Device::Get()->CreateBuffer(ImageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, true /*bHostVisible*/);

	void* MappedData = Device::Get()->MapBuffer(StagingImageBuffer);
	memcpy(MappedData, UploadPixels, static_cast<size_t>(ImageSize));
	Device::Get()->FlushBuffer(StagingImageBuffer);
	Device::Get()->UnmapBuffer(StagingImageBuffer);

	TexImageInfo = Device::Get()->CreateImage(mWidth, mHeight, UploadFormat, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);

	// Texture Image View
	VkImageViewCreateInfo TexImageViewInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
	TexImageViewInfo.image = TexImageInfo.Image;
	TexImageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	TexImageViewInfo.format = UploadFormat;
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
	// samplerAnisotropy is not enabled on the logical device, so keep it off.
	TexImageSamplerInfo.anisotropyEnable = VK_FALSE;
	TexImageSamplerInfo.maxAnisotropy = 1.f;
	TexImageSamplerInfo.compareEnable = VK_FALSE;
	TexImageSamplerInfo.compareOp = VK_COMPARE_OP_LESS;
	TexImageSamplerInfo.minLod = 0.f;
	TexImageSamplerInfo.maxLod = 0.f;
	TexImageSamplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	TexImageSamplerInfo.unnormalizedCoordinates = VK_FALSE;

	VKCall(vkCreateSampler(Device::Get()->GetDevice(), &TexImageSamplerInfo, nullptr, &TexSampler), "Failed to create Tex image sampler.");

	// Upload synchronously: the texture must be sampleable as soon as Init()
	// returns, and the staging buffer can then be destroyed immediately instead
	// of being kept alive until a later frame flushes the copy queue.
	CommandBufferVK* CopyCommandBuffer = Device::Get()->AllocateTransientCommandBuffer(CommandQueueType::Transfer);
	if (!CopyCommandBuffer)
	{
		std::cout << "[Texture] Could not allocate an upload command buffer for " << mFilePath << std::endl;
		return;
	}

	CopyCommandBuffer->Begin();
	VkCommandBuffer CmdBuf = CopyCommandBuffer->mCommandBuffer;

	// Transition image from UNDEFINED to TRANSFER_DST_OPTIMAL
	VkImageMemoryBarrier ToTransferBarrier = {};
	ToTransferBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	ToTransferBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	ToTransferBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	ToTransferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	ToTransferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	ToTransferBarrier.image = TexImageInfo.Image;
	ToTransferBarrier.srcAccessMask = 0;
	ToTransferBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	ToTransferBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	ToTransferBarrier.subresourceRange.baseMipLevel = 0;
	ToTransferBarrier.subresourceRange.levelCount = 1;
	ToTransferBarrier.subresourceRange.baseArrayLayer = 0;
	ToTransferBarrier.subresourceRange.layerCount = 1;

	vkCmdPipelineBarrier(CmdBuf,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
		0, 0, nullptr, 0, nullptr, 1, &ToTransferBarrier);

	// Copy buffer to image
	VkBufferImageCopy CopyRegion = {};
	CopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	CopyRegion.imageSubresource.layerCount = 1;
	CopyRegion.imageExtent.width = mWidth;
	CopyRegion.imageExtent.height = mHeight;
	CopyRegion.imageExtent.depth = 1;

	vkCmdCopyBufferToImage(CmdBuf, StagingImageBuffer.Buffer, TexImageInfo.Image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &CopyRegion);

	// Transition image from TRANSFER_DST_OPTIMAL to SHADER_READ_ONLY_OPTIMAL
	VkImageMemoryBarrier ToShaderBarrier = {};
	ToShaderBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	ToShaderBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	ToShaderBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	ToShaderBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	ToShaderBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	ToShaderBarrier.image = TexImageInfo.Image;
	ToShaderBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	ToShaderBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	ToShaderBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	ToShaderBarrier.subresourceRange.baseMipLevel = 0;
	ToShaderBarrier.subresourceRange.levelCount = 1;
	ToShaderBarrier.subresourceRange.baseArrayLayer = 0;
	ToShaderBarrier.subresourceRange.layerCount = 1;

	vkCmdPipelineBarrier(CmdBuf,
		VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		0, 0, nullptr, 0, nullptr, 1, &ToShaderBarrier);

	CopyCommandBuffer->End();
	Device::Get()->GetQueue(CommandQueueType::Transfer)->SubmitCommandBuffer(CopyCommandBuffer, true /*bWait*/);
	delete CopyCommandBuffer;

	Device::Get()->DestroyBuffer(StagingImageBuffer);
}

}
