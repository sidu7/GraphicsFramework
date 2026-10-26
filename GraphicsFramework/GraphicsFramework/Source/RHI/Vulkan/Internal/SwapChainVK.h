#pragma once

#include "RHI/Vulkan/DeviceVK.h"

namespace rhi::vk
{
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

class SwapChainVK
{
	friend class Helper;
public:
	SwapChainVK();
	virtual ~SwapChainVK();

	void Init();
	void AcquireNextImage(const SemaphoreVK* SignalSemaphore);

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
}
