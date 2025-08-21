#include <iostream>
#include <set>
#include <algorithm>
#define STB_IMAGE_IMPLEMENTATION
#include "Core/Core.h"
#include <SDL_vulkan.h>
#include <vulkan\include\vulkan\vulkan.h>
#include <vulkan/vk_enum_string_helper.h>
#include "Utils/FileHelper.h"

#define VKCall(FuncCall, Error) \
{	\
	VkResult Result = FuncCall; \
	if(Result != VK_SUCCESS) \
	{	\
		std::cout << "Vulkan Error:[" << string_VkResult(Result) << "] " << Error << std::endl; \
		__debugbreak(); \
	}	\
}

#define BACKBUFFER_COUNT (uint32_t)2
#define WIDTH 800
#define HEIGHT 600
SDL_Window* pWindow = nullptr;

const std::vector<const char*> DeviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

struct QueueFamilyIndices
{
	uint32_t GraphicsQueueIndex = std::numeric_limits<uint32_t>::max();
	uint32_t PresentQueueIndex = std::numeric_limits<uint32_t>::max();
	uint32_t TransferQueueIndex = std::numeric_limits<uint32_t>::max();

	bool IsValid()
	{
		return GraphicsQueueIndex != std::numeric_limits<uint32_t>::max() &&
			PresentQueueIndex != std::numeric_limits<uint32_t>::max() &&
			TransferQueueIndex != std::numeric_limits<uint32_t>::max();
	}
};

VkPhysicalDevice PhysicalDevice = VK_NULL_HANDLE;
VkDevice Device = VK_NULL_HANDLE;
VkSurfaceKHR Surface = VK_NULL_HANDLE;
QueueFamilyIndices QueueFamilyData;
VkQueue GraphicsQueue = VK_NULL_HANDLE;
VkQueue PresentQueue = VK_NULL_HANDLE;
VkQueue TransferQueue = VK_NULL_HANDLE;
VkSwapchainKHR SwapChain = VK_NULL_HANDLE;
VkSwapchainKHR OldSwapChain = VK_NULL_HANDLE;
std::vector<VkImage> SwapChainImages;
std::vector<VkImageView> SwapChainImageViews;
VkFormat SwapChainImageFormat = VK_FORMAT_UNDEFINED;
VkExtent2D SwapChainImageExtent = {};
VkRenderPass RenderPass = VK_NULL_HANDLE;
std::vector<VkFramebuffer> SwapchainFrameBuffers;
VkPhysicalDeviceMemoryProperties MemoryProperties;

struct SwapChainDetails
{
	VkSurfaceCapabilitiesKHR SurfaceCapabilities;
	std::vector<VkSurfaceFormatKHR> Formats;
	std::vector<VkPresentModeKHR> PresentModes;

	bool IsValid()
	{
		return !Formats.empty() && !PresentModes.empty();
	}

	VkExtent2D GetImageExtent()
	{
		if (SurfaceCapabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
		{
			return SurfaceCapabilities.currentExtent;
		}

		int Width, Height;
		SDL_Vulkan_GetDrawableSize(pWindow, &Width, &Height);

		VkExtent2D ImageExtent;
		ImageExtent.width = std::clamp((uint32_t)Width, SurfaceCapabilities.minImageExtent.width, SurfaceCapabilities.maxImageExtent.width);
		ImageExtent.height = std::clamp((uint32_t)Height, SurfaceCapabilities.minImageExtent.height, SurfaceCapabilities.maxImageExtent.height);

		return ImageExtent;
	}

	const VkSurfaceFormatKHR& GetSurfaceFormat()
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

	const VkPresentModeKHR GetPresentMode()
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
};

bool AreLayersAvailable(std::vector<const char*> Layers)
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

bool AreExtensionsAvailable(std::vector<const char*> Extensions)
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

bool AreDeviceExtensionsAvailable(std::vector<const char*> Extensions, VkPhysicalDevice PhysicalDevice)
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

VKAPI_ATTR VkBool32 VKAPI_CALL DebugMessageCallback(VkDebugUtilsMessageSeverityFlagBitsEXT Severity, VkDebugUtilsMessageTypeFlagsEXT MessageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
	if (pCallbackData)
	{
		if (Severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
		{
			return false;
		}

		const char* SeverityStr = "";
		switch (Severity)
		{
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
				SeverityStr = "Error";
				break;
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
				SeverityStr = "Warning";
				break;
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
				SeverityStr = "Info";
				break;
		}

		std::cout << "Vulkan " << SeverityStr << " " << pCallbackData->pMessage << std::endl;
	}

	return false;
}

void CreateDebugMessenger(const VkDebugUtilsMessengerCreateInfoEXT* CreateInfo, VkInstance Instance, VkDebugUtilsMessengerEXT* Messenger)
{
	PFN_vkCreateDebugUtilsMessengerEXT Func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(Instance, "vkCreateDebugUtilsMessengerEXT");
	if (Func != nullptr)
	{
		VKCall(Func(Instance, CreateInfo, nullptr, Messenger), "Debug Messenger creation Failed.");
	}
	else
	{
		std::cout << "vkCreateDebugUtilsMessengerEXT could not be found" << std::endl;
	}
}

SwapChainDetails GetSwapChainDetails(VkPhysicalDevice PhysicalDevice, VkSurfaceKHR Surface)
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

bool IsDeviceSupported(VkPhysicalDevice PhysicalDevice, VkSurfaceKHR Surface, QueueFamilyIndices* QueueFamilyData)
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
				QueueFamilyData->GraphicsQueueIndex = i;

				VkBool32 bPresentSupported = false;
				VKCall(vkGetPhysicalDeviceSurfaceSupportKHR(PhysicalDevice, i, Surface, &bPresentSupported), "Failed to get surface support");
				if (bPresentSupported)
				{
					QueueFamilyData->PresentQueueIndex = i;
				}
			}
			if ((QueueProperty.queueFlags & VK_QUEUE_TRANSFER_BIT) /*&& !(QueueProperty.queueFlags & VK_QUEUE_GRAPHICS_BIT)*/)
			{
				QueueFamilyData->TransferQueueIndex = i;
			}

			// Is device extension supported
			bool bExtensionsSupported = AreDeviceExtensionsAvailable(DeviceExtensions, PhysicalDevice);

			// Is swapchain supported
			SwapChainDetails SwapChainData = GetSwapChainDetails(PhysicalDevice, Surface);

			if (bExtensionsSupported && QueueFamilyData->IsValid() && SwapChainData.IsValid())
			{
				return true;
			}
		}
	}

	return false;
}

void FillSharingMode(VkSharingMode& sharingMode, uint32_t& queueFamilyIndicesCount, const uint32_t** queueFamilyIndices)
{
	if (QueueFamilyData.GraphicsQueueIndex != QueueFamilyData.PresentQueueIndex)
	{
		static const std::vector<uint32_t> Queues = { QueueFamilyData.GraphicsQueueIndex, QueueFamilyData.PresentQueueIndex };

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

VkPhysicalDevice GetDesiredPhysicalDevice(VkInstance Instance, VkSurfaceKHR Surface, QueueFamilyIndices* QueueFamilyData)
{
	uint32_t PhysicalDevicesCount;
	vkEnumeratePhysicalDevices(Instance, &PhysicalDevicesCount, nullptr);
	std::vector<VkPhysicalDevice> PhysicalDevices(PhysicalDevicesCount);
	vkEnumeratePhysicalDevices(Instance, &PhysicalDevicesCount, PhysicalDevices.data());

	for (const VkPhysicalDevice& Device : PhysicalDevices)
	{
		if (IsDeviceSupported(Device, Surface, QueueFamilyData))
		{
			return Device;
		}
	}

	return VK_NULL_HANDLE;
}

void DestroyDebugMessenger(VkDebugUtilsMessengerEXT DebugMessenger, VkInstance Instance)
{
	PFN_vkDestroyDebugUtilsMessengerEXT Func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(Instance, "vkDestroyDebugUtilsMessengerEXT");
	if (Func)
	{
		Func(Instance, DebugMessenger, nullptr);
	}
	else
	{
		std::cout << "vkCreateDebugUtilsMessengerEXT could not be found" << std::endl;
	}
}

void CreateFrameBuffers()
{
	SwapchainFrameBuffers.resize(SwapChainImageViews.size());
	for (int32_t i = 0; i < SwapChainImageViews.size(); ++i)
	{
		const VkImageView& ImageView = SwapChainImageViews[i];
		VkFramebufferCreateInfo FrameBufferCreateInfo = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
		FrameBufferCreateInfo.renderPass = RenderPass;
		FrameBufferCreateInfo.attachmentCount = 1;
		FrameBufferCreateInfo.pAttachments = &ImageView;
		FrameBufferCreateInfo.width = SwapChainImageExtent.width;
		FrameBufferCreateInfo.height = SwapChainImageExtent.height;
		FrameBufferCreateInfo.layers = 1;

		VKCall(vkCreateFramebuffer(Device, &FrameBufferCreateInfo, nullptr, &SwapchainFrameBuffers[i]), "Swapchain Framebuffer creation Failed.");
	}
}

void CreateSwapChain()
{
	SwapChainDetails SwapChainData = GetSwapChainDetails(PhysicalDevice, Surface);
	const VkSurfaceFormatKHR& SurfaceFormatData = SwapChainData.GetSurfaceFormat();

	VkSwapchainCreateInfoKHR SwapChainCreateInfo = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
	SwapChainCreateInfo.surface = Surface;
	SwapChainCreateInfo.minImageCount = std::clamp(BACKBUFFER_COUNT, SwapChainData.SurfaceCapabilities.minImageCount, SwapChainData.SurfaceCapabilities.maxImageCount);
	SwapChainCreateInfo.imageFormat = SurfaceFormatData.format;
	SwapChainImageFormat = SurfaceFormatData.format;
	SwapChainCreateInfo.imageColorSpace = SurfaceFormatData.colorSpace;
	SwapChainCreateInfo.imageExtent = SwapChainData.GetImageExtent();
	SwapChainImageExtent = SwapChainCreateInfo.imageExtent;
	SwapChainCreateInfo.imageArrayLayers = 1;
	SwapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT /*| VK_IMAGE_USAGE_TRANSFER_DST_BIT*/;
	FillSharingMode(SwapChainCreateInfo.imageSharingMode, SwapChainCreateInfo.queueFamilyIndexCount, &SwapChainCreateInfo.pQueueFamilyIndices);
	SwapChainCreateInfo.preTransform = SwapChainData.SurfaceCapabilities.currentTransform;
	SwapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	SwapChainCreateInfo.presentMode = SwapChainData.GetPresentMode();
	SwapChainCreateInfo.clipped = VK_TRUE;
	SwapChainCreateInfo.oldSwapchain = OldSwapChain;

	VKCall(vkCreateSwapchainKHR(Device, &SwapChainCreateInfo, nullptr, &SwapChain), "Swapchain creation failed");


	// Retrieve SwapChain image for rendering
	uint32_t SwapChainImageCount = 0;
	VKCall(vkGetSwapchainImagesKHR(Device, SwapChain, &SwapChainImageCount, nullptr), "Retriving SwapChain Image count failed");
	SwapChainImages.resize(SwapChainImageCount);
	VKCall(vkGetSwapchainImagesKHR(Device, SwapChain, &SwapChainImageCount, SwapChainImages.data()), "Retriving SwapChain Images failed");

	// Create Image Views for each swap chain image
	SwapChainImageViews.resize(SwapChainImages.size());
	for (int32_t i = 0; i < SwapChainImages.size(); ++i)
	{
		VkImageViewCreateInfo ViewCreateInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
		ViewCreateInfo.image = SwapChainImages[i];
		ViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		ViewCreateInfo.format = SwapChainImageFormat;
		ViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		ViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		ViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		ViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		ViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		ViewCreateInfo.subresourceRange.baseMipLevel = 0;
		ViewCreateInfo.subresourceRange.levelCount = 1;
		ViewCreateInfo.subresourceRange.baseArrayLayer = 0;
		ViewCreateInfo.subresourceRange.layerCount = 1;

		VKCall(vkCreateImageView(Device, &ViewCreateInfo, nullptr, &SwapChainImageViews[i]), "Failed to create Swap Chain Image view " << i);
	}
}

void DestroySwapChainBuffers()
{
	for (const VkFramebuffer& FrameBuffer : SwapchainFrameBuffers)
	{
		vkDestroyFramebuffer(Device, FrameBuffer, nullptr);
	}
	for (const VkImageView& ImageView : SwapChainImageViews)
	{
		vkDestroyImageView(Device, ImageView, nullptr);
	}

	SwapchainFrameBuffers.clear();
	SwapChainImageViews.clear();
	SwapChainImages.clear();
}

void RecreateSwapChain()
{
	vkDeviceWaitIdle(Device);

	DestroySwapChainBuffers();
	vkDestroySwapchainKHR(Device, SwapChain, nullptr);

	CreateSwapChain();
	CreateFrameBuffers();
}

uint32_t FindMemoryType(uint32_t Type, VkMemoryPropertyFlags Properties)
{
	for (int32_t i = 0; i < MemoryProperties.memoryTypeCount; ++i)
	{
		if (Type & (1 << i) && (MemoryProperties.memoryTypes[i].propertyFlags & Properties) == Properties)
		{
			return i;
		}
	}

	std::cout << "Vulkan Failed to find the desired memory type." << std::endl;
	return -1;
}

struct BufferInfo
{
	VkBuffer Buffer;
	VkDeviceMemory BufferMemory;
	void* MappedMemory;
};

BufferInfo CreateBuffer(VkDeviceSize BufferSize, VkBufferUsageFlags UsageFlags, VkMemoryPropertyFlags MemoryPropertyFlags)
{
	BufferInfo BufferData;

	VkBufferCreateInfo BufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	BufferInfo.size = BufferSize;
	BufferInfo.usage = UsageFlags;
	FillSharingMode(BufferInfo.sharingMode, BufferInfo.queueFamilyIndexCount, &BufferInfo.pQueueFamilyIndices);
	VKCall(vkCreateBuffer(Device, &BufferInfo, nullptr, &BufferData.Buffer), "Buffer creation Failed.");

	VkMemoryRequirements VBMemoryRequirements;
	vkGetBufferMemoryRequirements(Device, BufferData.Buffer, &VBMemoryRequirements);

	VkMemoryAllocateInfo AllocInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
	AllocInfo.allocationSize = VBMemoryRequirements.size;
	AllocInfo.memoryTypeIndex = FindMemoryType(VBMemoryRequirements.memoryTypeBits, MemoryPropertyFlags);

	VKCall(vkAllocateMemory(Device, &AllocInfo, nullptr, &BufferData.BufferMemory), "Buffer memory allocation Failed.");

	VKCall(vkBindBufferMemory(Device, BufferData.Buffer, BufferData.BufferMemory, 0), "Buffer memory binding Failed.");

	return BufferData;
}

int main(int argc, char* args[])
{
	// Init
	uint32_t error = 0;

	//Initialize SDL
	if ((error = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER)) < 0)//Init for video and joystick
	{
		printf("Couldn't initialize SDL, error %i\n", error);
		return 0;
	}
	SDL_Vulkan_LoadLibrary(nullptr);

	//printf("%i joysticks were found.\n\n", SDL_NumJoysticks());//NUmber of joysticks
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	// Enable AntiAliasiing
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

	pWindow = SDL_CreateWindow("Vulkan Framework",
							   SDL_WINDOWPOS_CENTERED,
							   SDL_WINDOWPOS_CENTERED,
							   WIDTH,
							   HEIGHT,
							   SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);

	//Check if window was made successfully
	if (pWindow == NULL)
	{
		printf("Could not create window: %s\n", SDL_GetError());
		return 0;
	}

	uint32_t AvailableExtensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &AvailableExtensionCount, nullptr);

	std::cout << AvailableExtensionCount << " extensions supported\n";

	bool bAddDebugExtensions = false;
#if _DEBUG
	bAddDebugExtensions = true;
#else 
	bAddDebugExtensions = false;
#endif

	uint32_t ExtensionCount;

	// Gather SDL extensions
	SDL_Vulkan_GetInstanceExtensions(pWindow, &ExtensionCount, nullptr);

	std::vector<const char*> Extensions(ExtensionCount);
	SDL_Vulkan_GetInstanceExtensions(pWindow, &ExtensionCount, Extensions.data());

	// Add debug extensions
	if (bAddDebugExtensions)
	{
		Extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	VkInstance Instance = VK_NULL_HANDLE;
	VkDebugUtilsMessengerEXT DebugMessenger = VK_NULL_HANDLE;

	const std::vector<const char*> Layers = {
		"VK_LAYER_KHRONOS_validation"
	};

	if (AreExtensionsAvailable(Extensions) && AreLayersAvailable(Layers))
	{
		// Create Instance
		VkApplicationInfo AppInfo = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
		AppInfo.pApplicationName = "Vulkan Test";
		AppInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		AppInfo.pEngineName = "Graphics Framework";
		AppInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		AppInfo.apiVersion = VK_API_VERSION_1_3;

		VkInstanceCreateInfo InstanceCreateInfo = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
		InstanceCreateInfo.pApplicationInfo = &AppInfo;

		VkDebugUtilsMessengerCreateInfoEXT DebugMessengerCreateInfo = { VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };

		if (bAddDebugExtensions)
		{
			// Create debug messenger
			DebugMessengerCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;

			DebugMessengerCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;

			DebugMessengerCreateInfo.pfnUserCallback = (PFN_vkDebugUtilsMessengerCallbackEXT)DebugMessageCallback;

			InstanceCreateInfo.enabledLayerCount = Layers.size();
			InstanceCreateInfo.ppEnabledLayerNames = Layers.data();
			InstanceCreateInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&DebugMessengerCreateInfo;
		}
		else
		{
			InstanceCreateInfo.pNext = nullptr;
		}

		InstanceCreateInfo.enabledExtensionCount = Extensions.size();
		InstanceCreateInfo.ppEnabledExtensionNames = Extensions.data();

		VKCall(vkCreateInstance(&InstanceCreateInfo, nullptr, &Instance), "Vulkan Instance creation failed.");

		if (bAddDebugExtensions)
		{
			CreateDebugMessenger(&DebugMessengerCreateInfo, Instance, &DebugMessenger);
		}
	}

	if (!Instance)
	{
		std::cout << "Vulkan Instance could not be created" << std::endl;
	}

	// Create Window Surface SDL
	if (!SDL_Vulkan_CreateSurface(pWindow, Instance, &Surface))
	{
		std::cout << "Failed to create SDL vulkan surface" << std::endl;
	}

	// Find the desired Physical Device
	PhysicalDevice = GetDesiredPhysicalDevice(Instance, Surface, &QueueFamilyData);

	if (!PhysicalDevice)
	{
		std::cout << "Vulkan Physical Device could not be found" << std::endl;
	}

	VkPhysicalDeviceProperties DeviceProperties = {};
	vkGetPhysicalDeviceProperties(PhysicalDevice, &DeviceProperties);

	// Get Memory Types
	vkGetPhysicalDeviceMemoryProperties(PhysicalDevice, &MemoryProperties);

	std::cout << "Vulkan Physical Device: " << DeviceProperties.deviceName << " is selected." << std::endl;

	// Create logical device
	std::set<uint32_t> UniqueQueues = { QueueFamilyData.GraphicsQueueIndex, QueueFamilyData.PresentQueueIndex, QueueFamilyData.TransferQueueIndex };

	std::vector<VkDeviceQueueCreateInfo> QueueCreateInfos;

	for (uint32_t QueueIdx : UniqueQueues)
	{
		VkDeviceQueueCreateInfo QueueCreateInfo = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
		float QueuePriority = 1.0f;
		QueueCreateInfo.pQueuePriorities = &QueuePriority;
		QueueCreateInfo.queueCount = 1;
		QueueCreateInfo.queueFamilyIndex = QueueIdx;
		QueueCreateInfos.push_back(QueueCreateInfo);
	}

	VkPhysicalDeviceFeatures DeviceFeatures = {};
	DeviceFeatures.samplerAnisotropy = VK_TRUE;

	VkDeviceCreateInfo DeviceCreateInfo = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
	DeviceCreateInfo.enabledExtensionCount = DeviceExtensions.size();
	DeviceCreateInfo.ppEnabledExtensionNames = DeviceExtensions.data();
	DeviceCreateInfo.enabledLayerCount = Layers.size();
	DeviceCreateInfo.ppEnabledLayerNames = Layers.data();
	DeviceCreateInfo.queueCreateInfoCount = QueueCreateInfos.size();
	DeviceCreateInfo.pQueueCreateInfos = QueueCreateInfos.data();
	DeviceCreateInfo.pEnabledFeatures = &DeviceFeatures;

	VKCall(vkCreateDevice(PhysicalDevice, &DeviceCreateInfo, nullptr, &Device), "Logical Device creation failed.");

	vkGetDeviceQueue(Device, QueueFamilyData.GraphicsQueueIndex, 0, &GraphicsQueue);
	vkGetDeviceQueue(Device, QueueFamilyData.PresentQueueIndex, 0, &PresentQueue);
	vkGetDeviceQueue(Device, QueueFamilyData.TransferQueueIndex, 0, &TransferQueue);

	// Create Swapchain
	CreateSwapChain();

	// Create Shader Stages
	std::vector<VkPipelineShaderStageCreateInfo> ShaderStageCreateInfos;
	VkShaderModule VertexShaderModule = VK_NULL_HANDLE;
	VkShaderModule FragmentShaderModule = VK_NULL_HANDLE;

	// Programmable stages
	{
		std::vector<char> VertexByteCode = FileHelper::ReadBinaryFile("Shader.vert.spv");
		std::vector<char> FragmentByteCode = FileHelper::ReadBinaryFile("Shader.frag.spv");

		VkShaderModuleCreateInfo VertexShaderModuleCreateInfo = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
		VertexShaderModuleCreateInfo.codeSize = VertexByteCode.size();
		VertexShaderModuleCreateInfo.pCode = (const uint32_t*)VertexByteCode.data();

		VKCall(vkCreateShaderModule(Device, &VertexShaderModuleCreateInfo, nullptr, &VertexShaderModule), "Vertex Shader Module creation Failed.");

		VkShaderModuleCreateInfo FragmentShaderModuleCreateInfo = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
		FragmentShaderModuleCreateInfo.codeSize = FragmentByteCode.size();
		FragmentShaderModuleCreateInfo.pCode = (const uint32_t*)FragmentByteCode.data();

		VKCall(vkCreateShaderModule(Device, &FragmentShaderModuleCreateInfo, nullptr, &FragmentShaderModule), "Fragment Shader Module creation Failed.");

		VkPipelineShaderStageCreateInfo VertexShaderStageCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
		VertexShaderStageCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		VertexShaderStageCreateInfo.module = VertexShaderModule;
		VertexShaderStageCreateInfo.pName = "main";

		ShaderStageCreateInfos.push_back(VertexShaderStageCreateInfo);

		VkPipelineShaderStageCreateInfo FragmentShaderStageCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
		FragmentShaderStageCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		FragmentShaderStageCreateInfo.module = FragmentShaderModule;
		FragmentShaderStageCreateInfo.pName = "main";

		ShaderStageCreateInfos.push_back(FragmentShaderStageCreateInfo);
	}

	VkViewport ViewPort = {};
	VkRect2D ScissorRect = {};
	VkCullModeFlags CullMode = VK_CULL_MODE_NONE;
	VkFrontFace FrontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	VkPrimitiveTopology Topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	VkBool32 DepthTestEnable = VK_FALSE;

	struct Vertex
	{
		glm::vec2 Position;
		glm::vec3 Color;
		glm::vec2 TexCoords;
	};

	VkVertexInputBindingDescription VertexBindingDesc;
	VertexBindingDesc.binding = 0;
	VertexBindingDesc.stride = sizeof(Vertex);
	VertexBindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	std::vector<VkVertexInputAttributeDescription> VertexAttribDescs(3);
	VertexAttribDescs[0].binding = 0;
	VertexAttribDescs[0].location = 0;
	VertexAttribDescs[0].format = VK_FORMAT_R32G32_SFLOAT;
	VertexAttribDescs[0].offset = offsetof(Vertex, Position);

	VertexAttribDescs[1].binding = 0;
	VertexAttribDescs[1].location = 1;
	VertexAttribDescs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	VertexAttribDescs[1].offset = offsetof(Vertex, Color);

	VertexAttribDescs[2].binding = 0;
	VertexAttribDescs[2].location = 2;
	VertexAttribDescs[2].format = VK_FORMAT_R32G32_SFLOAT;
	VertexAttribDescs[2].offset = offsetof(Vertex, TexCoords);

	// Fixed pipeline stages
	VkPipelineVertexInputStateCreateInfo VertexInputStageCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
	VertexInputStageCreateInfo.vertexBindingDescriptionCount = 1;
	VertexInputStageCreateInfo.pVertexBindingDescriptions = &VertexBindingDesc;
	VertexInputStageCreateInfo.vertexAttributeDescriptionCount = VertexAttribDescs.size();
	VertexInputStageCreateInfo.pVertexAttributeDescriptions = VertexAttribDescs.data();

	VkPipelineInputAssemblyStateCreateInfo InputAssemblyStateCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
	InputAssemblyStateCreateInfo.topology = Topology;
	InputAssemblyStateCreateInfo.primitiveRestartEnable = VK_FALSE;

	VkPipelineTessellationStateCreateInfo TessellationStateCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO };

	VkPipelineViewportStateCreateInfo ViewportStateCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
	ViewportStateCreateInfo.viewportCount = 1;
	ViewportStateCreateInfo.pViewports = &ViewPort;
	ViewportStateCreateInfo.scissorCount = 1;
	ViewportStateCreateInfo.pScissors = &ScissorRect;

	VkPipelineRasterizationStateCreateInfo RasterizationStateCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
	RasterizationStateCreateInfo.depthClampEnable = VK_FALSE;
	RasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;
	RasterizationStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
	RasterizationStateCreateInfo.cullMode = CullMode;
	RasterizationStateCreateInfo.frontFace = FrontFace;
	RasterizationStateCreateInfo.depthBiasEnable = VK_FALSE;
	RasterizationStateCreateInfo.depthBiasConstantFactor = 0.f;
	RasterizationStateCreateInfo.depthBiasClamp = 0.f;
	RasterizationStateCreateInfo.depthBiasSlopeFactor = 0.f;
	RasterizationStateCreateInfo.lineWidth = 1.f;

	VkPipelineMultisampleStateCreateInfo MultisampleStateCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
	MultisampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	MultisampleStateCreateInfo.sampleShadingEnable = VK_FALSE;
	MultisampleStateCreateInfo.minSampleShading = 1.f;
	MultisampleStateCreateInfo.pSampleMask = nullptr;
	MultisampleStateCreateInfo.alphaToCoverageEnable = VK_FALSE;
	MultisampleStateCreateInfo.alphaToOneEnable = VK_FALSE;

	VkPipelineDepthStencilStateCreateInfo DepthStencilStateCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
	DepthStencilStateCreateInfo.depthTestEnable = VK_FALSE;
	DepthStencilStateCreateInfo.depthWriteEnable = VK_FALSE;
	DepthStencilStateCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS;
	DepthStencilStateCreateInfo.depthBoundsTestEnable = VK_FALSE;
	DepthStencilStateCreateInfo.stencilTestEnable = VK_FALSE;
	DepthStencilStateCreateInfo.front = VkStencilOpState{};
	DepthStencilStateCreateInfo.back = VkStencilOpState{};
	DepthStencilStateCreateInfo.minDepthBounds = 0.f;
	DepthStencilStateCreateInfo.maxDepthBounds = 1.f;

	VkPipelineColorBlendAttachmentState ColorBlendAttachmentState = {};
	ColorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	ColorBlendAttachmentState.blendEnable = VK_FALSE;
	ColorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	ColorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	ColorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
	ColorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	ColorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	ColorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;

	VkPipelineColorBlendStateCreateInfo ColorBlendStateCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
	ColorBlendStateCreateInfo.logicOpEnable = VK_FALSE;
	ColorBlendStateCreateInfo.logicOp = VK_LOGIC_OP_COPY;
	ColorBlendStateCreateInfo.attachmentCount = 1;
	ColorBlendStateCreateInfo.pAttachments = &ColorBlendAttachmentState;
	ColorBlendStateCreateInfo.blendConstants[0] = 0.f;
	ColorBlendStateCreateInfo.blendConstants[1] = 0.f;
	ColorBlendStateCreateInfo.blendConstants[2] = 0.f;
	ColorBlendStateCreateInfo.blendConstants[3] = 0.f;

	const std::vector<VkDynamicState> DynamicState = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR,
		VK_DYNAMIC_STATE_CULL_MODE,
		VK_DYNAMIC_STATE_FRONT_FACE,
		VK_DYNAMIC_STATE_PRIMITIVE_TOPOLOGY,
		VK_DYNAMIC_STATE_DEPTH_TEST_ENABLE,
	};

	VkPipelineDynamicStateCreateInfo DynamicStateCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
	DynamicStateCreateInfo.dynamicStateCount = DynamicState.size();
	DynamicStateCreateInfo.pDynamicStates = DynamicState.data();

	// Create Render Pass
	VkAttachmentDescription AttachmentDesc = {};
	AttachmentDesc.format = SwapChainImageFormat;
	AttachmentDesc.samples = VK_SAMPLE_COUNT_1_BIT;
	AttachmentDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	AttachmentDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	AttachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	AttachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	AttachmentDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // image state before subpass
	AttachmentDesc.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // image state after subpass

	VkAttachmentReference ColorAttachmentRef = {};
	ColorAttachmentRef.attachment = 0;
	ColorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // image state during the subpass

	VkSubpassDescription SubpassDesc = {};
	SubpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	SubpassDesc.inputAttachmentCount = 0;
	SubpassDesc.pInputAttachments = nullptr;
	SubpassDesc.colorAttachmentCount = 1;
	SubpassDesc.pColorAttachments = &ColorAttachmentRef;
	SubpassDesc.pResolveAttachments = nullptr;
	SubpassDesc.pDepthStencilAttachment = nullptr;
	SubpassDesc.preserveAttachmentCount = 0;
	SubpassDesc.pPreserveAttachments = nullptr;

	VkSubpassDependency SubpassDependency = {};
	SubpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	SubpassDependency.dstSubpass = 0;
	SubpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	SubpassDependency.srcAccessMask = 0;
	SubpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	SubpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo RenderPassCreateInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
	RenderPassCreateInfo.attachmentCount = 1;
	RenderPassCreateInfo.pAttachments = &AttachmentDesc;
	RenderPassCreateInfo.subpassCount = 1;
	RenderPassCreateInfo.pSubpasses = &SubpassDesc;
	RenderPassCreateInfo.dependencyCount = 1;
	RenderPassCreateInfo.pDependencies = &SubpassDependency;

	VKCall(vkCreateRenderPass(Device, &RenderPassCreateInfo, nullptr, &RenderPass), "Render Pass creation Failed.");

	// Ubo Descriptor Set layout
	VkDescriptorSetLayoutBinding UboBinding = {};
	UboBinding.binding = 0;
	UboBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	UboBinding.descriptorCount = 1;
	UboBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	UboBinding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutBinding SamplerBinding = {};
	SamplerBinding.binding = 1;
	SamplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	SamplerBinding.descriptorCount = 1;
	SamplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	SamplerBinding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayout DescLayout = VK_NULL_HANDLE;

	std::vector<VkDescriptorSetLayoutBinding> Bindings = { UboBinding , SamplerBinding };
	VkDescriptorSetLayoutCreateInfo DescLayoutInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
	DescLayoutInfo.bindingCount = Bindings.size();
	DescLayoutInfo.pBindings = Bindings.data();

	VKCall(vkCreateDescriptorSetLayout(Device, &DescLayoutInfo, nullptr, &DescLayout), "Descriptor Set Layout creation Failed.");

	// Create Pipeline Layout
	VkPipelineLayout PipelineLayout = VK_NULL_HANDLE;

	VkPipelineLayoutCreateInfo PipelineCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	PipelineCreateInfo.flags = VK_PIPELINE_BIND_POINT_GRAPHICS;
	PipelineCreateInfo.setLayoutCount = 1;
	PipelineCreateInfo.pSetLayouts = &DescLayout;
	PipelineCreateInfo.pushConstantRangeCount = 0;
	PipelineCreateInfo.pPushConstantRanges = nullptr;

	VKCall(vkCreatePipelineLayout(Device, &PipelineCreateInfo, nullptr, &PipelineLayout), "Pipeline Layout creation Failed.");

	// Create Graphics Pipeline
	VkGraphicsPipelineCreateInfo GraphicsPipeLineCreateInfo = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
	GraphicsPipeLineCreateInfo.stageCount = ShaderStageCreateInfos.size();
	GraphicsPipeLineCreateInfo.pStages = ShaderStageCreateInfos.data();
	GraphicsPipeLineCreateInfo.pVertexInputState = &VertexInputStageCreateInfo;
	GraphicsPipeLineCreateInfo.pInputAssemblyState = &InputAssemblyStateCreateInfo;
	GraphicsPipeLineCreateInfo.pTessellationState = &TessellationStateCreateInfo;
	GraphicsPipeLineCreateInfo.pViewportState = &ViewportStateCreateInfo;
	GraphicsPipeLineCreateInfo.pRasterizationState = &RasterizationStateCreateInfo;
	GraphicsPipeLineCreateInfo.pMultisampleState = &MultisampleStateCreateInfo;
	GraphicsPipeLineCreateInfo.pDepthStencilState = &DepthStencilStateCreateInfo;
	GraphicsPipeLineCreateInfo.pColorBlendState = &ColorBlendStateCreateInfo;
	GraphicsPipeLineCreateInfo.pDynamicState = &DynamicStateCreateInfo;
	GraphicsPipeLineCreateInfo.layout = PipelineLayout;
	GraphicsPipeLineCreateInfo.renderPass = RenderPass;
	GraphicsPipeLineCreateInfo.subpass = 0;
	GraphicsPipeLineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
	GraphicsPipeLineCreateInfo.basePipelineIndex = -1;

	VkPipeline GraphicsPipeline = VK_NULL_HANDLE;
	VKCall(vkCreateGraphicsPipelines(Device, VK_NULL_HANDLE, 1, &GraphicsPipeLineCreateInfo, nullptr, &GraphicsPipeline), "Graphics Pipeline creation Failed.");

	// Destroy shader modules as graphics pipeline is created
	vkDestroyShaderModule(Device, VertexShaderModule, nullptr);
	vkDestroyShaderModule(Device, FragmentShaderModule, nullptr);

	CreateFrameBuffers();

	// Normal Command Buffer Pool
	VkCommandPool CommandBufferPool;

	VkCommandPoolCreateInfo CommandPoolCreateInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
	CommandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	CommandPoolCreateInfo.queueFamilyIndex = QueueFamilyData.GraphicsQueueIndex;

	VKCall(vkCreateCommandPool(Device, &CommandPoolCreateInfo, nullptr, &CommandBufferPool), "Command Buffer Pool creation Failed.");

	std::vector<VkCommandBuffer> CommandBuffers(BACKBUFFER_COUNT);

	VkCommandBufferAllocateInfo CommandBufferAllocateInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	CommandBufferAllocateInfo.commandPool = CommandBufferPool;
	CommandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	CommandBufferAllocateInfo.commandBufferCount = CommandBuffers.size();

	VKCall(vkAllocateCommandBuffers(Device, &CommandBufferAllocateInfo, CommandBuffers.data()), "Command Buffer allocation Failed.");

	// Transient Command Buffer Pool
	VkCommandPool TransientBufferPool;

	VkCommandPoolCreateInfo TransientCommandPoolCreateInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
	TransientCommandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
	TransientCommandPoolCreateInfo.queueFamilyIndex = QueueFamilyData.TransferQueueIndex;
	
	VKCall(vkCreateCommandPool(Device, &TransientCommandPoolCreateInfo, nullptr, &TransientBufferPool), "Transient Command Buffer Pool creation Failed.");

	uint32_t ImageIdx = 0;
	uint32_t FrameIdx = 0;

	std::vector<VkSemaphore> ImageAvailableSemaphores(BACKBUFFER_COUNT);
	std::vector<VkSemaphore> RenderingFinishedSemaphores(BACKBUFFER_COUNT);
	std::vector<VkFence> InFlightFences(BACKBUFFER_COUNT);

	VkSemaphoreCreateInfo SemaphoreCreateInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };

	VkFenceCreateInfo FenceCreateInfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
	FenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (int32_t i = 0; i < BACKBUFFER_COUNT; ++i)
	{
		VKCall(vkCreateSemaphore(Device, &SemaphoreCreateInfo, nullptr, &ImageAvailableSemaphores[i]), "Image Available Semaphore creation Failed");
		VKCall(vkCreateSemaphore(Device, &SemaphoreCreateInfo, nullptr, &RenderingFinishedSemaphores[i]), "Rendering Finished Semaphore creation Failed.");

		VKCall(vkCreateFence(Device, &FenceCreateInfo, nullptr, &InFlightFences[i]), "Previous Frame Fence creation Failed");
	}

	// Texture Image
	int32_t Width, Height, Channels;
	unsigned char* ImagePixels = stbi_load("Texture_1.jpg", &Width, &Height, &Channels, STBI_rgb_alpha);
	uint32_t ImageSize = Width * Height * 4;
	BufferInfo ImageStagingBuffer = CreateBuffer(ImageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	vkMapMemory(Device, ImageStagingBuffer.BufferMemory, 0, ImageSize, 0, &ImageStagingBuffer.MappedMemory);
	memcpy(ImageStagingBuffer.MappedMemory, ImagePixels, static_cast<size_t>(ImageSize));
	vkUnmapMemory(Device, ImageStagingBuffer.BufferMemory);

	stbi_image_free(ImagePixels);

	VkImage TexImage;
	VkDeviceMemory TexImageMemory;

	VkImageCreateInfo TexImageInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
	TexImageInfo.imageType = VK_IMAGE_TYPE_2D;
	TexImageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
	TexImageInfo.extent.width = Width;
	TexImageInfo.extent.height = Height;
	TexImageInfo.extent.depth = 1;
	TexImageInfo.mipLevels = 1;
	TexImageInfo.arrayLayers = 1;
	TexImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	TexImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	TexImageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	FillSharingMode(TexImageInfo.sharingMode, TexImageInfo.queueFamilyIndexCount, &TexImageInfo.pQueueFamilyIndices);
	TexImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	VKCall(vkCreateImage(Device, &TexImageInfo, nullptr, &TexImage), "Failed to create Texture Image.");

	VkMemoryRequirements TexImageMemReq;
	vkGetImageMemoryRequirements(Device, TexImage, &TexImageMemReq);

	VkMemoryAllocateInfo TexImageAllocInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
	TexImageAllocInfo.allocationSize = TexImageMemReq.size;
	TexImageAllocInfo.memoryTypeIndex = FindMemoryType(TexImageMemReq.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	VKCall(vkAllocateMemory(Device, &TexImageAllocInfo, nullptr, &TexImageMemory), "Failed to allocate image memory.");

	VKCall(vkBindImageMemory(Device, TexImage, TexImageMemory, 0), "Failed to bind image memory.");

	// Texture Image View
	VkImageView TexImageView;

	VkImageViewCreateInfo TexImageViewInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
	TexImageViewInfo.image = TexImage;
	TexImageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	TexImageViewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
	TexImageViewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	TexImageViewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	TexImageViewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	TexImageViewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
	TexImageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	TexImageViewInfo.subresourceRange.baseArrayLayer = 0;
	TexImageViewInfo.subresourceRange.baseMipLevel = 0;
	TexImageViewInfo.subresourceRange.layerCount = 1;
	TexImageViewInfo.subresourceRange.levelCount = 1;

	VKCall(vkCreateImageView(Device, &TexImageViewInfo, nullptr, &TexImageView), "Failed to create Image view.");

	// Texture Sampler
	VkSampler TexImageSampler;

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

	VKCall(vkCreateSampler(Device, &TexImageSamplerInfo, nullptr, &TexImageSampler), "Failed to create Tex image sampler.");

	// Vertex Buffer
	const std::vector<Vertex> Vertices = {
		{ glm::vec2(-0.5f, -0.5f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec2(1.f, 0.f)},
		{ glm::vec2(0.5f, -0.5f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(0.f, 0.f) },
		{ glm::vec2(0.5f, 0.5f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0.f, 1.f) },
		{ glm::vec2(-0.5f, 0.5f), glm::vec3(1.0f, 1.0f, 1.0f), glm::vec2(1.f, 1.f) }
	};

	uint32_t VertexDataSize = sizeof(Vertex) * Vertices.size();

	BufferInfo StagingVertexBuffer = CreateBuffer(VertexDataSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);;
	BufferInfo VertexBuffer = CreateBuffer(VertexDataSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	// Map Memory and copy data
	VKCall(vkMapMemory(Device, StagingVertexBuffer.BufferMemory, 0, VertexDataSize, 0, &StagingVertexBuffer.MappedMemory), "Staging Vertex Buffer memory mapping Failed.");
	memcpy(StagingVertexBuffer.MappedMemory, Vertices.data(), VertexDataSize);
	vkUnmapMemory(Device, StagingVertexBuffer.BufferMemory);

	// Index Buffer
	const std::vector<uint16_t> Indices = {	0, 1, 2, 2, 3, 0 };

	uint32_t IndexDataSize = sizeof(uint16_t) * Indices.size();

	BufferInfo StagingIndexBuffer = CreateBuffer(IndexDataSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);;
	BufferInfo IndexBuffer = CreateBuffer(IndexDataSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	// Map Memory and copy data
	VKCall(vkMapMemory(Device, StagingIndexBuffer.BufferMemory, 0, IndexDataSize, 0, &StagingIndexBuffer.MappedMemory), "Staging Index Buffer memory mapping Failed.");
	memcpy(StagingIndexBuffer.MappedMemory , Indices.data(), IndexDataSize);
	vkUnmapMemory(Device, StagingIndexBuffer.BufferMemory);

	// Copy from Staging to Buffer
	VkCommandBuffer CopyCommandBuffer;

	VkCommandBufferAllocateInfo CopyCommandBufferInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	CopyCommandBufferInfo.commandPool = TransientBufferPool;
	CopyCommandBufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	CopyCommandBufferInfo.commandBufferCount = 1;

	VKCall(vkAllocateCommandBuffers(Device, &CopyCommandBufferInfo, &CopyCommandBuffer), "Copy Command Buffer allocation Failed.");

	VkCommandBufferBeginInfo CopyCommandInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	CopyCommandInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	VKCall(vkBeginCommandBuffer(CopyCommandBuffer, &CopyCommandInfo), "Begin Copy Command Buffer Failed.");

	// Transition to Transfer dest
	VkImageMemoryBarrier ImageMemoryBarrierCpy = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
	ImageMemoryBarrierCpy.srcAccessMask = 0;
	ImageMemoryBarrierCpy.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	ImageMemoryBarrierCpy.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	ImageMemoryBarrierCpy.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	ImageMemoryBarrierCpy.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	ImageMemoryBarrierCpy.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	ImageMemoryBarrierCpy.image = TexImage;
	ImageMemoryBarrierCpy.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	ImageMemoryBarrierCpy.subresourceRange.baseArrayLayer = 0;
	ImageMemoryBarrierCpy.subresourceRange.baseMipLevel = 0;
	ImageMemoryBarrierCpy.subresourceRange.layerCount = 1;
	ImageMemoryBarrierCpy.subresourceRange.levelCount = 1;

	vkCmdPipelineBarrier(CopyCommandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &ImageMemoryBarrierCpy);

	// Copy image data from staged
	VkBufferImageCopy ImageCopyInfo = {};
	ImageCopyInfo.bufferOffset = 0;
	ImageCopyInfo.bufferRowLength = 0;
	ImageCopyInfo.bufferImageHeight = 0;
	ImageCopyInfo.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	ImageCopyInfo.imageSubresource.baseArrayLayer = 0;
	ImageCopyInfo.imageSubresource.layerCount = 1;
	ImageCopyInfo.imageSubresource.mipLevel = 0;
	ImageCopyInfo.imageOffset = { 0,0,0 };
	ImageCopyInfo.imageExtent.width = Width;
	ImageCopyInfo.imageExtent.height = Height;
	ImageCopyInfo.imageExtent.depth = 1;

	vkCmdCopyBufferToImage(CopyCommandBuffer, ImageStagingBuffer.Buffer, TexImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &ImageCopyInfo);

	// Transfer to shader readable
	VkImageMemoryBarrier ImageMemoryBarrierShdr = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
	ImageMemoryBarrierShdr.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	ImageMemoryBarrierShdr.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	ImageMemoryBarrierShdr.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	ImageMemoryBarrierShdr.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	ImageMemoryBarrierShdr.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	ImageMemoryBarrierShdr.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	ImageMemoryBarrierShdr.image = TexImage;
	ImageMemoryBarrierShdr.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	ImageMemoryBarrierShdr.subresourceRange.baseArrayLayer = 0;
	ImageMemoryBarrierShdr.subresourceRange.baseMipLevel = 0;
	ImageMemoryBarrierShdr.subresourceRange.layerCount = 1;
	ImageMemoryBarrierShdr.subresourceRange.levelCount = 1;

	vkCmdPipelineBarrier(CopyCommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &ImageMemoryBarrierShdr);

	VkBufferCopy CopyRegion = {};
	CopyRegion.dstOffset = 0;
	CopyRegion.srcOffset = 0;
	CopyRegion.size = VertexDataSize;

	// Copy vertex buffer
	vkCmdCopyBuffer(CopyCommandBuffer, StagingVertexBuffer.Buffer, VertexBuffer.Buffer, 1, &CopyRegion);

	CopyRegion.size = IndexDataSize;

	// Copy index buffer
	vkCmdCopyBuffer(CopyCommandBuffer, StagingIndexBuffer.Buffer, IndexBuffer.Buffer, 1, &CopyRegion);

	VKCall(vkEndCommandBuffer(CopyCommandBuffer), "End Copy Command Buffer Failed.");

	VkFence CopyFence = VK_NULL_HANDLE;

	VkFenceCreateInfo CopyFenceInfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };

	VKCall(vkCreateFence(Device, &CopyFenceInfo, nullptr, &CopyFence), "Copy Fence creation Failed.");

	VkSubmitInfo CopySubmitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
	CopySubmitInfo.commandBufferCount = 1;
	CopySubmitInfo.pCommandBuffers = &CopyCommandBuffer;

	vkQueueSubmit(TransferQueue, 1, &CopySubmitInfo, CopyFence);

	VKCall(vkWaitForFences(Device, 1, &CopyFence, VK_TRUE, UINT64_MAX), "Waiting for Copy Fence Failed.");

	vkDestroyFence(Device, CopyFence, nullptr);
	
	vkDestroyBuffer(Device, StagingVertexBuffer.Buffer, nullptr);
	vkFreeMemory(Device, StagingVertexBuffer.BufferMemory, nullptr);

	vkDestroyBuffer(Device, StagingIndexBuffer.Buffer, nullptr);
	vkFreeMemory(Device, StagingIndexBuffer.BufferMemory, nullptr);

	vkFreeMemory(Device, ImageStagingBuffer.BufferMemory, nullptr);
	vkDestroyBuffer(Device, ImageStagingBuffer.Buffer, nullptr);

	// Uniform Buffer
	struct UniformBufferObject
	{
		glm::mat4 Perspective;
		glm::mat4 View;
		glm::mat4 Model;
	};

	UniformBufferObject UboData;
	UboData.Perspective = glm::perspective(glm::radians(45.f), SwapChainImageExtent.width / (float)SwapChainImageExtent.height, 0.1f, 100.0f);
	UboData.Perspective[1][1] *= -1.f;
	UboData.View = glm::lookAt(glm::vec3(1.f, 1.f, 1.f), glm::vec3(0.f), glm::vec3(0.f, 0.f, 1.f));
	UboData.Model = glm::mat4(1.0f);

	std::vector<BufferInfo> UniformBuffers(BACKBUFFER_COUNT);
	
	for (uint16_t i = 0; i < BACKBUFFER_COUNT; ++i)
	{
		UniformBuffers[i] = CreateBuffer(sizeof(UniformBufferObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		VKCall(vkMapMemory(Device, UniformBuffers[i].BufferMemory, 0, sizeof(UniformBufferObject), 0, &UniformBuffers[i].MappedMemory), "Uniform Buffer memory mapping Failed.");
	}

	// Descriptor Pool
	VkDescriptorPool DescPool;

	std::vector<VkDescriptorPoolSize> DescPoolSizes(2);
	DescPoolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	DescPoolSizes[0].descriptorCount = BACKBUFFER_COUNT;

	DescPoolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	DescPoolSizes[1].descriptorCount = BACKBUFFER_COUNT;

	VkDescriptorPoolCreateInfo DescPoolInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
	DescPoolInfo.maxSets = BACKBUFFER_COUNT;
	DescPoolInfo.poolSizeCount = DescPoolSizes.size();
	DescPoolInfo.pPoolSizes = DescPoolSizes.data();

	VKCall(vkCreateDescriptorPool(Device, &DescPoolInfo, nullptr, &DescPool), "Desciptor Pool creation Failed.");

	std::vector<VkDescriptorSet> DescSet(BACKBUFFER_COUNT);

	std::vector<VkDescriptorSetLayout> Layouts(BACKBUFFER_COUNT, DescLayout);
	VkDescriptorSetAllocateInfo DescAllocInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
	DescAllocInfo.descriptorPool = DescPool;
	DescAllocInfo.descriptorSetCount = Layouts.size();
	DescAllocInfo.pSetLayouts = Layouts.data();

	VKCall(vkAllocateDescriptorSets(Device, &DescAllocInfo, DescSet.data()), "Ubo Descriptor Set allocation Failed.");

	for (int32_t i = 0; i < BACKBUFFER_COUNT; ++i)
	{
		VkDescriptorBufferInfo UboDescBufferInfo = {};
		UboDescBufferInfo.buffer = UniformBuffers[i].Buffer;
		UboDescBufferInfo.offset = 0;
		UboDescBufferInfo.range = sizeof(UniformBufferObject);

		VkDescriptorImageInfo SamplerDescInfo = {};
		SamplerDescInfo.sampler = TexImageSampler;
		SamplerDescInfo.imageView = TexImageView;
		SamplerDescInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		std::vector<VkWriteDescriptorSet> WriteDescs(2);
		WriteDescs[0] = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
		WriteDescs[0].dstSet = DescSet[i];
		WriteDescs[0].dstBinding = 0;
		WriteDescs[0].dstArrayElement = 0;
		WriteDescs[0].descriptorCount = 1;
		WriteDescs[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		WriteDescs[0].pBufferInfo = &UboDescBufferInfo;

		WriteDescs[1] = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
		WriteDescs[1].dstSet = DescSet[i];
		WriteDescs[1].dstBinding = 1;
		WriteDescs[1].dstArrayElement = 0;
		WriteDescs[1].descriptorCount = 1;
		WriteDescs[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		WriteDescs[1].pImageInfo = &SamplerDescInfo;

		vkUpdateDescriptorSets(Device, WriteDescs.size(), WriteDescs.data(), 0, nullptr);
	}

	bool appIsRunning = true;
	bool bWindowMinized = false;
	auto startTime = std::chrono::high_resolution_clock::now();
	while (appIsRunning)
	{
		bool bWindowResized = false;
		SDL_Event e;
		while (SDL_PollEvent(&e) != 0)
		{
			if (e.type == SDL_QUIT)
			{
				appIsRunning = false;
			}
			else if (e.type == SDL_WINDOWEVENT)
			{
				switch (e.window.event)
				{
					case SDL_WINDOWEVENT_RESIZED:
					case SDL_WINDOWEVENT_MAXIMIZED:
						bWindowResized = true;
					break;
					
					case SDL_WINDOWEVENT_MINIMIZED:
						bWindowMinized = true;
					break;

					case SDL_WINDOWEVENT_RESTORED:
						bWindowMinized = false;
					break;
				}
			}
		}

		if (bWindowResized)
		{
			RecreateSwapChain();
			continue;
		}

		if (bWindowMinized)
		{
			continue;
		}

		VKCall(vkWaitForFences(Device, 1, &InFlightFences[FrameIdx], VK_TRUE, UINT64_MAX), "Waiting for Fence failed.");

		VkResult AcquireResult = vkAcquireNextImageKHR(Device, SwapChain, UINT64_MAX, ImageAvailableSemaphores[FrameIdx], VK_NULL_HANDLE, &ImageIdx);

		if (AcquireResult == VK_ERROR_OUT_OF_DATE_KHR || AcquireResult == VK_SUBOPTIMAL_KHR)
		{
			RecreateSwapChain();
			continue;
		}
		else if (AcquireResult != VK_SUCCESS)
		{
			std::cout << "Vulkan Error: [" << string_VkResult(AcquireResult) << "] Failed to Acquire Next Swapchain Image." << std::endl;
		}

		VKCall(vkResetFences(Device, 1, &InFlightFences[FrameIdx]), "Fence Reset Failed.");

		VKCall(vkResetCommandBuffer(CommandBuffers[FrameIdx], 0), "Command Buffer Reset Failed.");

		// Record Command Buffer
		VkCommandBufferBeginInfo CommandBufferBeginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
		CommandBufferBeginInfo.flags = 0;
		CommandBufferBeginInfo.pInheritanceInfo = nullptr;

		VKCall(vkBeginCommandBuffer(CommandBuffers[FrameIdx], &CommandBufferBeginInfo), "Command Buffer begin Failed.");

		VkRenderPassBeginInfo RenderPassBeginInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
		RenderPassBeginInfo.renderPass = RenderPass;
		RenderPassBeginInfo.framebuffer = SwapchainFrameBuffers[ImageIdx];
		RenderPassBeginInfo.renderArea.extent = SwapChainImageExtent;
		RenderPassBeginInfo.renderArea.offset = { 0, 0 };
		RenderPassBeginInfo.clearValueCount = 1;
		VkClearValue ClearColor = {};
		ClearColor.color = { 0.0f, 0.0f, 0.0f, 1.0f };
		RenderPassBeginInfo.pClearValues = &ClearColor;

		vkCmdBeginRenderPass(CommandBuffers[FrameIdx], &RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(CommandBuffers[FrameIdx], VK_PIPELINE_BIND_POINT_GRAPHICS, GraphicsPipeline);

		ViewPort.x = 0.0f;
		ViewPort.y = 0.0f;
		ViewPort.width = SwapChainImageExtent.width;
		ViewPort.height = SwapChainImageExtent.height;
		ViewPort.minDepth = 0.0f;
		ViewPort.maxDepth = 1.0f;

		ScissorRect.extent = SwapChainImageExtent;
		ScissorRect.offset = { 0, 0 };

		vkCmdSetViewport(CommandBuffers[FrameIdx], 0, 1, &ViewPort);
		vkCmdSetScissor(CommandBuffers[FrameIdx], 0, 1, &ScissorRect);
		vkCmdSetPrimitiveTopology(CommandBuffers[FrameIdx], Topology);
		vkCmdSetCullMode(CommandBuffers[FrameIdx], CullMode);
		vkCmdSetFrontFace(CommandBuffers[FrameIdx], FrontFace);
		vkCmdSetDepthTestEnable(CommandBuffers[FrameIdx], DepthTestEnable);

		VkBuffer Buffers[] = { VertexBuffer.Buffer };
		VkDeviceSize Offsets[] = {0};
		vkCmdBindVertexBuffers(CommandBuffers[FrameIdx], 0, ARRAY_SIZE(Buffers), Buffers, Offsets);
		vkCmdBindIndexBuffer(CommandBuffers[FrameIdx], IndexBuffer.Buffer, 0, VK_INDEX_TYPE_UINT16);
		
		// Update Model Matrix
		auto currentTime = std::chrono::high_resolution_clock::now();
		float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

		UboData.Model = glm::rotate(glm::mat4(1.f), time * glm::radians(90.f), glm::vec3(0.f, 0.f, 1.f));

		memcpy(UniformBuffers[FrameIdx].MappedMemory, &UboData, sizeof(UboData));

		vkCmdBindDescriptorSets(CommandBuffers[FrameIdx], VK_PIPELINE_BIND_POINT_GRAPHICS, PipelineLayout, 0, 1, &DescSet[FrameIdx], 0, nullptr);

		//vkCmdDraw(CommandBuffers[FrameIdx], Vertices.size(), 1, 0, 0);
		vkCmdDrawIndexed(CommandBuffers[FrameIdx], Indices.size(), 1, 0, 0, 0);

		vkCmdEndRenderPass(CommandBuffers[FrameIdx]);

		VKCall(vkEndCommandBuffer(CommandBuffers[FrameIdx]), "Command Buffer end Failed.");

		// Submit Command Buffer
		VkSubmitInfo SubmitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };

		VkSemaphore WaitSemaphores[] = { ImageAvailableSemaphores[FrameIdx] };
		VkPipelineStageFlags WaitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

		VkSemaphore SignalSemaphores[] = { RenderingFinishedSemaphores[FrameIdx] };

		SubmitInfo.waitSemaphoreCount = ARRAY_SIZE(WaitSemaphores);
		SubmitInfo.pWaitSemaphores = WaitSemaphores;
		SubmitInfo.pWaitDstStageMask = WaitStages;
		SubmitInfo.signalSemaphoreCount = ARRAY_SIZE(SignalSemaphores);
		SubmitInfo.pSignalSemaphores = SignalSemaphores;
		SubmitInfo.commandBufferCount = 1;
		SubmitInfo.pCommandBuffers = &CommandBuffers[FrameIdx];

		VKCall(vkQueueSubmit(GraphicsQueue, 1, &SubmitInfo, InFlightFences[FrameIdx]), "Command Buffer Submit Failed.");

		VkPresentInfoKHR PresentInfo = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
		PresentInfo.waitSemaphoreCount = ARRAY_SIZE(SignalSemaphores);
		PresentInfo.pWaitSemaphores = SignalSemaphores;
		PresentInfo.swapchainCount = 1;
		PresentInfo.pSwapchains = &SwapChain;
		PresentInfo.pImageIndices = &ImageIdx;
		PresentInfo.pResults = nullptr;
		VkResult PresentResult = vkQueuePresentKHR(PresentQueue, &PresentInfo);

		if (PresentResult == VK_ERROR_OUT_OF_DATE_KHR || PresentResult == VK_SUBOPTIMAL_KHR)
		{
			RecreateSwapChain();
		}
		else if (PresentResult != VK_SUCCESS)
		{
			std::cout << "Vulkan Error: [" << string_VkResult(AcquireResult) << "] Queue Present Image Failed." << std::endl;
		}

		FrameIdx = (FrameIdx + 1) % BACKBUFFER_COUNT;
	}

	vkDeviceWaitIdle(Device);

	vkFreeMemory(Device, TexImageMemory, nullptr);
	vkDestroyImage(Device, TexImage, nullptr);
	vkDestroyImageView(Device, TexImageView, nullptr);
	vkDestroySampler(Device, TexImageSampler, nullptr);

	vkDestroyDescriptorPool(Device, DescPool, nullptr);

	vkDestroyDescriptorSetLayout(Device, DescLayout, nullptr);
	
	vkDestroyBuffer(Device, VertexBuffer.Buffer, nullptr);
	vkFreeMemory(Device, VertexBuffer.BufferMemory, nullptr);

	vkDestroyBuffer(Device, IndexBuffer.Buffer, nullptr);
	vkFreeMemory(Device, IndexBuffer.BufferMemory, nullptr);

	for (uint16_t i = 0; i < BACKBUFFER_COUNT; ++i)
	{
		vkUnmapMemory(Device, UniformBuffers[i].BufferMemory);
		vkFreeMemory(Device, UniformBuffers[i].BufferMemory, nullptr);
		vkDestroyBuffer(Device, UniformBuffers[i].Buffer, nullptr);		
	}

	vkDestroyCommandPool(Device, CommandBufferPool, nullptr);
	vkDestroyCommandPool(Device, TransientBufferPool, nullptr);

	DestroySwapChainBuffers();

	vkDestroySwapchainKHR(Device, OldSwapChain, nullptr);
	vkDestroySwapchainKHR(Device, SwapChain, nullptr);

	vkDestroyPipeline(Device, GraphicsPipeline, nullptr);
	vkDestroyRenderPass(Device, RenderPass, nullptr);
	vkDestroyPipelineLayout(Device, PipelineLayout, nullptr);

	for (int32_t i = 0; i < BACKBUFFER_COUNT; ++i)
	{
		vkDestroySemaphore(Device, ImageAvailableSemaphores[i], nullptr);
		vkDestroySemaphore(Device, RenderingFinishedSemaphores[i], nullptr);
		vkDestroyFence(Device, InFlightFences[i], nullptr);
	}

	vkDestroySurfaceKHR(Instance, Surface, nullptr);
	vkDestroyDevice(Device, nullptr);

	DestroyDebugMessenger(DebugMessenger, Instance);
	vkDestroyInstance(Instance, nullptr);

	SDL_Vulkan_UnloadLibrary();
	SDL_DestroyWindow(pWindow);
	SDL_Quit();

	// Cleanup
}