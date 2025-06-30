#include "SwapChain_Vulkan.h"

#include <SDL_vulkan.h>
#include "Core/Window.h"
#include "Semaphore_Vulkan.h"

bool SwapChainDetails::IsValid()
{
	return !Formats.empty() && !PresentModes.empty();
}

VkExtent2D SwapChainDetails::GetImageExtent()
{
	if (SurfaceCapabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
	{
		return SurfaceCapabilities.currentExtent;
	}

	int Width, Height;
	SDL_Vulkan_GetDrawableSize(Window::Instance()->GetWindow(), &Width, &Height);

	VkExtent2D ImageExtent;
	ImageExtent.width = std::clamp((uint32_t)Width, SurfaceCapabilities.minImageExtent.width, SurfaceCapabilities.maxImageExtent.width);
	ImageExtent.height = std::clamp((uint32_t)Height, SurfaceCapabilities.minImageExtent.height, SurfaceCapabilities.maxImageExtent.height);

	return ImageExtent;
}

const VkSurfaceFormatKHR& SwapChainDetails::GetSurfaceFormat()
{
	for (const VkSurfaceFormatKHR& FormatData : Formats)
	{
		if (FormatData.colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR)
		{
			return FormatData;
		}
	}

	return Formats[0];
}

const VkPresentModeKHR SwapChainDetails::GetPresentMode()
{
	for (VkPresentModeKHR PresentMode : PresentModes)
	{
		if (PresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			return PresentMode;
		}
	}

	return VK_PRESENT_MODE_FIFO_KHR;
}


SwapChain_Vulkan::SwapChain_Vulkan() :
	SwapChain(VK_NULL_HANDLE),
	ImageFormat(VK_FORMAT_UNDEFINED),
	ImageExtent{0, 0},
	ImageIdx(0)
{
}

SwapChain_Vulkan::~SwapChain_Vulkan()
{
	for (const VkImageView& ImageView : ImageViews)
	{
		vkDestroyImageView(Renderer_Vulkan::Get()->GetDevice(), ImageView, nullptr);
	}

	vkDestroySwapchainKHR(Renderer_Vulkan::Get()->GetDevice(), SwapChain, nullptr);
}

void SwapChain_Vulkan::Init()
{
	Renderer_Vulkan* VkRenderer = Renderer_Vulkan::Get();

	SwapChainInfo = GetSwapChainDetails(VkRenderer->PhysicalDevice, VkRenderer->Surface);

	if (SwapChainInfo.IsValid())
	{
		VkSurfaceFormatKHR SurfaceFormat = SwapChainInfo.GetSurfaceFormat();

		VkSwapchainCreateInfoKHR SwapChainCreateInfo = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
		SwapChainCreateInfo.surface = VkRenderer->Surface;
		SwapChainCreateInfo.minImageCount = BACKBUFFER_COUNT;
		SwapChainCreateInfo.imageFormat = SurfaceFormat.format;
		ImageFormat = SwapChainCreateInfo.imageFormat;
		SwapChainCreateInfo.imageColorSpace = SurfaceFormat.colorSpace;
		SwapChainCreateInfo.imageExtent = SwapChainInfo.GetImageExtent();
		ImageExtent = SwapChainCreateInfo.imageExtent;
		SwapChainCreateInfo.imageArrayLayers = 1;
		SwapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		SwapChainCreateInfo.preTransform = SwapChainInfo.SurfaceCapabilities.currentTransform;
		SwapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		SwapChainCreateInfo.presentMode = SwapChainInfo.GetPresentMode();
		SwapChainCreateInfo.clipped = VK_TRUE;
		SwapChainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

		if (VkRenderer->QueueFamilyData.QueueIndices[CommandQueueType::Graphics] != VkRenderer->QueueFamilyData.QueueIndices[CommandQueueType::Present])
		{
			const std::vector<uint32_t> QueueFamilyIndices = { VkRenderer->QueueFamilyData.QueueIndices[CommandQueueType::Graphics], VkRenderer->QueueFamilyData.QueueIndices[CommandQueueType::Present] };
			SwapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			SwapChainCreateInfo.queueFamilyIndexCount = QueueFamilyIndices.size();
			SwapChainCreateInfo.pQueueFamilyIndices = QueueFamilyIndices.data();
		}
		else
		{
			SwapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			SwapChainCreateInfo.queueFamilyIndexCount = 0;
			SwapChainCreateInfo.pQueueFamilyIndices = nullptr;
		}

		VKCall(vkCreateSwapchainKHR(VkRenderer->Device, &SwapChainCreateInfo, nullptr, &SwapChain), "SwapChain creation Failed.");

		// Get SwapChain Images
		uint32_t SwapChainImagesCount = 0;
		VKCall(vkGetSwapchainImagesKHR(VkRenderer->Device, SwapChain, &SwapChainImagesCount, nullptr), "Swapchain Images Count retrival Failed.");
		Images.resize(SwapChainImagesCount);
		VKCall(vkGetSwapchainImagesKHR(VkRenderer->Device, SwapChain, &SwapChainImagesCount, Images.data()), "Swapchain Images retrival Failed.");

		// Create Image Views
		ImageViews.resize(SwapChainImagesCount);
		for (uint16_t i = 0; i < Images.size(); ++i)
		{
			VkImageViewCreateInfo ImageViewInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
			ImageViewInfo.image = Images[i];
			ImageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			ImageViewInfo.format = ImageFormat;
			ImageViewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			ImageViewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			ImageViewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			ImageViewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
			ImageViewInfo.subresourceRange.layerCount = 1;
			ImageViewInfo.subresourceRange.baseArrayLayer = 0;
			ImageViewInfo.subresourceRange.levelCount = 1;
			ImageViewInfo.subresourceRange.baseMipLevel = 0;
			ImageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

			VKCall(vkCreateImageView(VkRenderer->Device, &ImageViewInfo, nullptr, &ImageViews[i]), "Swapchain Image View creation Failed.");
		}
	}
}

void SwapChain_Vulkan::AcquireNextImage(const Semaphore_Vulkan* SignalSemaphore)
{
	VkResult AcquireResult = vkAcquireNextImageKHR(Renderer_Vulkan::Get()->GetDevice(), SwapChain, UINT64_MAX, SignalSemaphore->mSemaphore, VK_NULL_HANDLE, &ImageIdx);
}

SwapChainDetails SwapChain_Vulkan::GetSwapChainDetails(VkPhysicalDevice PhysicalDevice, VkSurfaceKHR Surface)
{
	SwapChainDetails Details;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(PhysicalDevice, Surface, &Details.SurfaceCapabilities);

	uint32_t SurfaceFormatCount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(PhysicalDevice, Surface, &SurfaceFormatCount, nullptr);
	if (SurfaceFormatCount > 0)
	{
		Details.Formats.resize(SurfaceFormatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(PhysicalDevice, Surface, &SurfaceFormatCount, Details.Formats.data());
	}

	uint32_t SurfacePresentModesCount = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(PhysicalDevice, Surface, &SurfacePresentModesCount, nullptr);
	if (SurfacePresentModesCount > 0)
	{
		Details.PresentModes.resize(SurfacePresentModesCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(PhysicalDevice, Surface, &SurfacePresentModesCount, Details.PresentModes.data());
	}

	return Details;
}
