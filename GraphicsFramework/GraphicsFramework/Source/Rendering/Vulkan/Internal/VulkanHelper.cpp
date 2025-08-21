#include "VulkanHelper.h"

#include "Core/Core.h"
#include "SDL_vulkan.h"
#include "Core/Window.h"
#include <vulkan/vk_enum_string_helper.h>
#include <Imgui\imgui_impl_vulkan.h>

#include "CommandBufferPool_Vulkan.h"
#include "CommandBuffer_Vulkan.h"
#include "CommandQueue_Vulkan.h"
#include "RenderPass_Vulkan.h"
#include "SwapChain_Vulkan.h"

// Required Layers
const std::vector<const char*> Layers = {
#if _DEBUG
	"VK_LAYER_KHRONOS_validation"
#endif
};

bool VulkanHelper::AreLayersAvailable(std::vector<const char*> Layers)
{
	uint32_t AvailableLayerCount = 0;
	vkEnumerateInstanceLayerProperties(&AvailableLayerCount, nullptr);
	std::vector<VkLayerProperties> AvailableLayers(AvailableLayerCount);
	vkEnumerateInstanceLayerProperties(&AvailableLayerCount, AvailableLayers.data());

	for (const char* LayerName : Layers)
	{
		bool LayerFound = false;
		for (const VkLayerProperties& AvailableLayer : AvailableLayers)
		{
			if (strcmp(LayerName, AvailableLayer.layerName) == 0)
			{
				LayerFound = true;
				break;
			}
		}

		if (!LayerFound)
		{
			std::cout << "Vulkan Layer : " << LayerName << " Not available." << std::endl;
			return false;
		}
	}

	return true;
}

bool VulkanHelper::AreExtensionsAvailable(std::vector<const char*> Extensions)
{
	uint32_t AvailableExtensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &AvailableExtensionCount, nullptr);
	std::vector<VkExtensionProperties> AvailableExtensions(AvailableExtensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &AvailableExtensionCount, AvailableExtensions.data());

	for (const char* ExtensionName : Extensions)
	{
		bool ExtensionFound = false;
		for (const VkExtensionProperties& AvailableExtension : AvailableExtensions)
		{
			if (strcmp(ExtensionName, AvailableExtension.extensionName) == 0)
			{
				ExtensionFound = true;
				break;
			}
		}

		if (!ExtensionFound)
		{
			std::cout << "Vulkan Extension : " << ExtensionFound << " Not available." << std::endl;
			return false;
		}
	}

	return true;
}

bool VulkanHelper::AreDeviceExtensionsAvailable(std::vector<const char*> Extensions, VkPhysicalDevice PhysicalDevice)
{
	uint32_t AvailableExtensionCount = 0;
	vkEnumerateDeviceExtensionProperties(PhysicalDevice, nullptr, &AvailableExtensionCount, nullptr);
	std::vector<VkExtensionProperties> AvailableExtensions(AvailableExtensionCount);
	vkEnumerateDeviceExtensionProperties(PhysicalDevice, nullptr, &AvailableExtensionCount, AvailableExtensions.data());

	for (const char* ExtensionName : Extensions)
	{
		bool ExtensionFound = false;
		for (const VkExtensionProperties& AvailableExtension : AvailableExtensions)
		{
			if (strcmp(ExtensionName, AvailableExtension.extensionName) == 0)
			{
				ExtensionFound = true;
				break;
			}
		}

		if (!ExtensionFound)
		{
			std::cout << "Vulkan Device Extension : " << ExtensionFound << " Not available." << std::endl;
			return false;
		}
	}

	return true;
}

bool VulkanHelper::IsDeviceSupported(VkPhysicalDevice PhysicalDevice, VkSurfaceKHR Surface, QueueFamilyIndices* OutQueueFamilyData)
{
	// Device supported checks
	VkPhysicalDeviceProperties DeviceProperties = {};
	vkGetPhysicalDeviceProperties(PhysicalDevice, &DeviceProperties);

	// if (DeviceProperties.deviceType == VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) // Dedicated GPU
	{
		uint32_t QueueFamilyPropertiesCount;
		vkGetPhysicalDeviceQueueFamilyProperties(PhysicalDevice, &QueueFamilyPropertiesCount, nullptr);
		std::vector<VkQueueFamilyProperties> QueueFamilyProperties(QueueFamilyPropertiesCount);
		vkGetPhysicalDeviceQueueFamilyProperties(PhysicalDevice, &QueueFamilyPropertiesCount, QueueFamilyProperties.data());

		for (int32_t i = 0; i < QueueFamilyPropertiesCount; ++i)
		{
			const VkQueueFamilyProperties& QueueProperty = QueueFamilyProperties[i];
			if (QueueProperty.queueFlags & VK_QUEUE_GRAPHICS_BIT) // Has Graphics Queue support
			{
				OutQueueFamilyData->QueueIndices[CommandQueueType::Graphics] = i;

				VkBool32 bPresentSupported = false;
				VKCall(vkGetPhysicalDeviceSurfaceSupportKHR(PhysicalDevice, i, Surface, &bPresentSupported), "Failed to get surface support");
				if (bPresentSupported)
				{
					OutQueueFamilyData->QueueIndices[CommandQueueType::Present] = i;
				}
			}
			if ((QueueProperty.queueFlags & VK_QUEUE_TRANSFER_BIT) && !(QueueProperty.queueFlags & VK_QUEUE_GRAPHICS_BIT))
			{
				OutQueueFamilyData->QueueIndices[CommandQueueType::Transfer] = i;
			}
			if (QueueProperty.queueFlags & VK_QUEUE_COMPUTE_BIT)
			{
				OutQueueFamilyData->QueueIndices[CommandQueueType::Compute] = i;
			}

			// Is device extension supported
			Renderer_Vulkan* VulkanRenderer = static_cast<Renderer_Vulkan*>(Renderer::Instance());
			bool bExtensionsSupported = AreDeviceExtensionsAvailable(VulkanRenderer->DeviceExtensions, PhysicalDevice);

			// Is swapchain supported
			SwapChainDetails SwapChainData = SwapChain_Vulkan::GetSwapChainDetails(PhysicalDevice, Surface);

			if (bExtensionsSupported && OutQueueFamilyData->IsValid() && SwapChainData.IsValid())
			{
				return true;
			}
		}
	}

	return false;
}

VkPhysicalDevice VulkanHelper::GetDesiredPhysicalDevice(VkInstance Instance, VkSurfaceKHR Surface, QueueFamilyIndices* OutQueueFamilyData)
{
	uint32_t PhysicalDevicesCount;
	vkEnumeratePhysicalDevices(Instance, &PhysicalDevicesCount, nullptr);
	std::vector<VkPhysicalDevice> PhysicalDevices(PhysicalDevicesCount);
	vkEnumeratePhysicalDevices(Instance, &PhysicalDevicesCount, PhysicalDevices.data());

	for (const VkPhysicalDevice& Device : PhysicalDevices)
	{
		if (IsDeviceSupported(Device, Surface, OutQueueFamilyData))
		{
			return Device;
		}
	}

	return VK_NULL_HANDLE;
}

void VulkanHelper::CreateDebugMessenger()
{
	Renderer_Vulkan* VkRenderer = static_cast<Renderer_Vulkan*>(Renderer::Instance());
	if (VkRenderer)
	{
		PFN_vkCreateDebugUtilsMessengerEXT Func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(VkRenderer->VulkanInstance, "vkCreateDebugUtilsMessengerEXT");
		if (Func)
		{
			VkDebugUtilsMessengerCreateInfoEXT DebugMessengerInfo = { VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
			DebugMessengerInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
			DebugMessengerInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
			DebugMessengerInfo.pfnUserCallback = Renderer_Vulkan::DebugMessageCallback;
			DebugMessengerInfo.pUserData = VkRenderer;

			VKCall(Func(VkRenderer->VulkanInstance, &DebugMessengerInfo, nullptr, &VkRenderer->DebugMessenger), "Debug Messenger creation Failed");
		}
		else
		{
			std::cout << "Could not find vkDestroyDebugUtilsMessengerEXT function" << std::endl;
		}
	}
}

void VulkanHelper::DestroyDebugMessenger(VkDebugUtilsMessengerEXT DebugMessenger)
{
	Renderer_Vulkan* VkRenderer = static_cast<Renderer_Vulkan*>(Renderer::Instance());
	if (VkRenderer)
	{
		PFN_vkDestroyDebugUtilsMessengerEXT Func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(VkRenderer->VulkanInstance, "vkDestroyDebugUtilsMessengerEXT");
		if (Func)
		{
			Func(VkRenderer->VulkanInstance, DebugMessenger, nullptr);
		}
		else
		{
			std::cout << "Could not find vkDestroyDebugUtilsMessengerEXT function" << std::endl;
		}
	}
}

void VulkanHelper::FillSharingMode(VkSharingMode& sharingMode, uint32_t& queueFamilyIndicesCount, const uint32_t** queueFamilyIndices)
{
	uint32_t GraphicsQueueIdx = Renderer_Vulkan::Get()->GetQueueFamilyIndex(CommandQueueType::Graphics);
	uint32_t TransferQueueIdx = Renderer_Vulkan::Get()->GetQueueFamilyIndex(CommandQueueType::Transfer);
	if (GraphicsQueueIdx != TransferQueueIdx)
	{
		static const std::vector<uint32_t> Queues = { GraphicsQueueIdx, TransferQueueIdx };

		sharingMode = VK_SHARING_MODE_CONCURRENT;
		queueFamilyIndicesCount = Queues.size();
		*queueFamilyIndices = Queues.data();
	}
	else
	{
		sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		queueFamilyIndicesCount = 0;
		*queueFamilyIndices = nullptr;
	}
}

void VulkanHelper::InitVulkan()
{
	Renderer_Vulkan* VkRenderer = static_cast<Renderer_Vulkan*>(Renderer::Instance());

	if (VkRenderer)
	{
		unsigned int SDLExtensionsCount = 0;
		SDL_Vulkan_GetInstanceExtensions(Window::Instance()->GetWindow(), &SDLExtensionsCount, nullptr);
		VkRenderer->InstanceExtensions.resize(SDLExtensionsCount);
		SDL_Vulkan_GetInstanceExtensions(Window::Instance()->GetWindow(), &SDLExtensionsCount, VkRenderer->InstanceExtensions.data());

#if _DEBUG
		VkRenderer->InstanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

		if (AreLayersAvailable(Layers) && AreExtensionsAvailable(VkRenderer->InstanceExtensions))
		{
			// Create Instance
			VkApplicationInfo AppInfo = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
			AppInfo.pApplicationName = "Vulkan Graphics Framework";
			AppInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
			AppInfo.pEngineName = "Graphics Framework";
			AppInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
			AppInfo.apiVersion = VK_API_VERSION_1_3;

			VkDebugUtilsMessengerCreateInfoEXT DebugMessengerInfo = { VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
			DebugMessengerInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
			DebugMessengerInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
			DebugMessengerInfo.pfnUserCallback = Renderer_Vulkan::DebugMessageCallback;
			DebugMessengerInfo.pUserData = VkRenderer;

			VkInstanceCreateInfo InstanceInfo = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
			InstanceInfo.pApplicationInfo = &AppInfo;
			InstanceInfo.enabledLayerCount = Layers.size();
			InstanceInfo.ppEnabledLayerNames = Layers.data();
			InstanceInfo.enabledExtensionCount = VkRenderer->InstanceExtensions.size();
			InstanceInfo.ppEnabledExtensionNames = VkRenderer->InstanceExtensions.data();
			InstanceInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&DebugMessengerInfo;

			VKCall(vkCreateInstance(&InstanceInfo, nullptr, &VkRenderer->VulkanInstance), "Vulkan Instance creation Failed.");

#if _DEBUG
			// Setup Debug Messenger 
			CreateDebugMessenger();
#endif

			// Create Surface
			if (!SDL_Vulkan_CreateSurface(Window::Instance()->GetWindow(), VkRenderer->VulkanInstance, &VkRenderer->Surface))
			{
				std::cout << "SDL Vulkan Surface Creation Failed" << std::endl;
			}

			// Select Physical Device and get it's Queue Family Index Data
			VkRenderer->PhysicalDevice = GetDesiredPhysicalDevice(VkRenderer->VulkanInstance, VkRenderer->Surface, &VkRenderer->QueueFamilyData);

			if (!VkRenderer->PhysicalDevice)
			{
				std::cout << "Physical Device selection Failed" << std::endl;
			}

			// Create logical device
			const std::set<unsigned int> UniqueQueueIndices = {
				VkRenderer->QueueFamilyData.QueueIndices[CommandQueueType::Graphics],
				VkRenderer->QueueFamilyData.QueueIndices[CommandQueueType::Compute],
				VkRenderer->QueueFamilyData.QueueIndices[CommandQueueType::Present],
				VkRenderer->QueueFamilyData.QueueIndices[CommandQueueType::Transfer]
			};

			// Get Physical Device Memory Properties
			vkGetPhysicalDeviceMemoryProperties(VkRenderer->PhysicalDevice, &VkRenderer->MemoryProperties);

			std::vector<VkDeviceQueueCreateInfo> QueueInfos;
			for (unsigned int QueueIndex : UniqueQueueIndices)
			{
				VkDeviceQueueCreateInfo QueueInfo = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
				QueueInfo.queueFamilyIndex = QueueIndex;
				QueueInfo.queueCount = 1;
				float Priority = 1.0f;
				QueueInfo.pQueuePriorities = &Priority;

				QueueInfos.push_back(QueueInfo);
			}

			if (AreDeviceExtensionsAvailable(VkRenderer->DeviceExtensions, VkRenderer->PhysicalDevice))
			{
				VkRenderer->DeviceExtensions = std::vector<const char*>{
					VK_KHR_SWAPCHAIN_EXTENSION_NAME,
				};

				const std::vector<const char*> DeviceLayers = {

				};

				VkDeviceCreateInfo DeviceInfo = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
				DeviceInfo.queueCreateInfoCount = QueueInfos.size();
				DeviceInfo.pQueueCreateInfos = QueueInfos.data();
				DeviceInfo.enabledExtensionCount = VkRenderer->DeviceExtensions.size();
				DeviceInfo.ppEnabledExtensionNames = VkRenderer->DeviceExtensions.data();
				DeviceInfo.enabledLayerCount = DeviceLayers.size();
				DeviceInfo.ppEnabledLayerNames = DeviceLayers.data();
				DeviceInfo.pEnabledFeatures = nullptr;

				VKCall(vkCreateDevice(VkRenderer->PhysicalDevice, &DeviceInfo, nullptr, &VkRenderer->Device), "Vulkan Logical Device creation Failed.");
			}
		}
	}
}

void VulkanHelper::InitImGui()
{
	// 1: create descriptor pool for IMGUI
	//  the size of the pool is very oversize, but it's copied from imgui demo
	//  itself.
	VkDescriptorPoolSize ImGuiDescPoolSizes[] = { { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 } };

	VkDescriptorPoolCreateInfo ImGuiDescPoolInfo = {};
	ImGuiDescPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	ImGuiDescPoolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	ImGuiDescPoolInfo.maxSets = 1000;
	ImGuiDescPoolInfo.poolSizeCount = (uint32_t)std::size(ImGuiDescPoolSizes);
	ImGuiDescPoolInfo.pPoolSizes = ImGuiDescPoolSizes;

	VKCall(vkCreateDescriptorPool(Renderer_Vulkan::Get()->GetDevice(), &ImGuiDescPoolInfo, nullptr, &Renderer_Vulkan::Get()->ImGuiDescPool), "ImGui Descriptor Pool creation Failed.");

	ImGui_ImplVulkan_InitInfo InitInfo = {};
	InitInfo.ApiVersion = VK_API_VERSION_1_3;
	InitInfo.Instance = Renderer_Vulkan::Get()->GetVulkanInstance();
	InitInfo.PhysicalDevice = Renderer_Vulkan::Get()->GetPhysicalDevice();
	InitInfo.Device = Renderer_Vulkan::Get()->GetDevice();
	InitInfo.QueueFamily = Renderer_Vulkan::Get()->GetQueueFamilyIndex(CommandQueueType::Graphics);
	InitInfo.Queue = Renderer_Vulkan::Get()->GetQueue(CommandQueueType::Graphics)->mQueue;
	InitInfo.DescriptorPool = Renderer_Vulkan::Get()->ImGuiDescPool;
	InitInfo.RenderPass = Renderer_Vulkan::Get()->ImGuiRenderPass->mRenderPass;
	InitInfo.MinImageCount = BACKBUFFER_COUNT;
	InitInfo.ImageCount = BACKBUFFER_COUNT;
	InitInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

	ImGui_ImplVulkan_Init(&InitInfo);

	CommandBuffer_Vulkan* GrCmdBuffer = Renderer_Vulkan::Get()->AllocateTransientCommandBuffer(CommandQueueType::Graphics);
	GrCmdBuffer->Begin();
	ImGui_ImplVulkan_CreateFontsTexture();
	GrCmdBuffer->End();
	Renderer_Vulkan::Get()->GetQueue(CommandQueueType::Graphics)->SubmitCommandBuffer(GrCmdBuffer, true /*bWait*/);
	delete GrCmdBuffer;
}
