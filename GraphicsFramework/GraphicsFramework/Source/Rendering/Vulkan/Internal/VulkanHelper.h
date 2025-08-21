#include "Rendering/Vulkan/Renderer_Vulkan.h"

class VulkanHelper
{
public:
	void InitVulkan();
	void InitImGui();

	void DestroyDebugMessenger(VkDebugUtilsMessengerEXT DebugMessenger);

	static void FillSharingMode(VkSharingMode& sharingMode, uint32_t& queueFamilyIndicesCount, const uint32_t** queueFamilyIndices);

protected:
	bool AreLayersAvailable(std::vector<const char*> Layers);
	bool AreExtensionsAvailable(std::vector<const char*> Extensions);
	bool AreDeviceExtensionsAvailable(std::vector<const char*> Extensions, VkPhysicalDevice PhysicalDevice);
	bool IsDeviceSupported(VkPhysicalDevice PhysicalDevice, VkSurfaceKHR Surface, QueueFamilyIndices* OutQueueFamilyData);
	VkPhysicalDevice GetDesiredPhysicalDevice(VkInstance Instance, VkSurfaceKHR Surface, QueueFamilyIndices* OutQueueFamilyData);

	void CreateDebugMessenger();
};