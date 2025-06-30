#include "Rendering/Vulkan/Renderer_Vulkan.h"

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
	bool IsDeviceSupported(VkPhysicalDevice PhysicalDevice, VkSurfaceKHR Surface, QueueFamilyIndices* OutQueueFamilyData);
	VkPhysicalDevice GetDesiredPhysicalDevice(VkInstance Instance, VkSurfaceKHR Surface, QueueFamilyIndices* OutQueueFamilyData);

	void CreateDebugMessenger();
};