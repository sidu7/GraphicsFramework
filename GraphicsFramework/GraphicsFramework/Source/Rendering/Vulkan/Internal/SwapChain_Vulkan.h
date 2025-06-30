#pragma once

#include "Rendering/Vulkan/Renderer_Vulkan.h"

struct SwapChainDetails
{
	VkSurfaceCapabilitiesKHR SurfaceCapabilities;
	std::vector<VkSurfaceFormatKHR> Formats;
	std::vector<VkPresentModeKHR> PresentModes;

	bool IsValid();
	VkExtent2D GetImageExtent();
	const VkSurfaceFormatKHR& GetSurfaceFormat();
	const VkPresentModeKHR GetPresentMode();
};

class SwapChain_Vulkan
{
	friend class VulkanHelper;
public:
	SwapChain_Vulkan();
	virtual ~SwapChain_Vulkan();

	void Init();
	void AcquireNextImage(const Semaphore_Vulkan* SignalSemaphore);

	VkSwapchainKHR SwapChain;
	std::vector<VkImage> Images;
	std::vector<VkImageView> ImageViews;
	VkFormat ImageFormat;
	VkExtent2D ImageExtent;

	uint32_t ImageIdx;

protected:
	static SwapChainDetails GetSwapChainDetails(VkPhysicalDevice PhysicalDevice, VkSurfaceKHR Surface);
	
	SwapChainDetails SwapChainInfo;
};