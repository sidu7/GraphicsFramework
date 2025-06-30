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

class VulkanHelper
{
public:
	void InitVulkan();
	void InitImGui();

	void DestroyDebugMessenger(VkDebugUtilsMessengerEXT DebugMessenger);

protected:
	bool AreLayersAvailable(std::vector<const char*> Layers);
	bool AreExtensionsAvailable(std::vector<const char*> Extensions);
	bool AreDeviceExtensionsAvailable(std::vector<const char*> Extensions, VkPhysicalDevice PhysicalDevice);
	SwapChainDetails GetSwapChainDetails(VkPhysicalDevice PhysicalDevice, VkSurfaceKHR Surface);
	bool IsDeviceSupported(VkPhysicalDevice PhysicalDevice, VkSurfaceKHR Surface, QueueFamilyIndices* OutQueueFamilyData);
	VkPhysicalDevice GetDesiredPhysicalDevice(VkInstance Instance, VkSurfaceKHR Surface, QueueFamilyIndices* OutQueueFamilyData);

	void CreateDebugMessenger();

	void CreateSwapChain(Renderer_Vulkan* VkRenderer);
};