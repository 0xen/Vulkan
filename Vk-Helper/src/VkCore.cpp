#include <VkCore.hpp>
#include <VkInitializers.hpp>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <memory>
#include <fstream>

// A basic debug callback. A more advanced one could be created, but this will do for basic debugging
VKAPI_ATTR VkBool32 VKAPI_CALL MyDebugReportCallback(
	VkDebugReportFlagsEXT       flags,
	VkDebugReportObjectTypeEXT  objectType,
	uint64_t                    object,
	size_t                      location,
	int32_t                     messageCode,
	const char*                 pLayerPrefix,
	const char*                 pMessage,
	void*                       pUserData)
{
	printf("%s\n",pMessage);
	return VK_FALSE;
}


// Compare the required layers to the avaliable layers on the system
bool VkHelper::CheckLayersSupport(const char** layers, int count)
{
	// Find out how many layers are avaliable on the system
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	// Using the count, tell the system how many layer definitions we want to read
	// These layer properties are the layers that are avaliable on the system
	std::unique_ptr<VkLayerProperties[]> layerProperties(new VkLayerProperties[layerCount]());
	vkEnumerateInstanceLayerProperties(&layerCount, layerProperties.get());

	// Loop through for each layer we want to check
	for (int i = 0; i < count; ++i)
	{
		bool layerFound = false;
		// Loop through for each avaliable system layer and atempt to find our required layer
		for (int j = 0; j < layerCount; ++j)
		{
			// Check to see if the layer matches
			if (strcmp(layers[i], layerProperties[j].layerName) == 0)
			{
				layerFound = true;
				break;
			}
		}
		// If we are missing the required layer, report back
		if (!layerFound)
		{
			return false;
		}
	}
	// Found all the layers
	return true;
}

VkInstance VkHelper::CreateInstance(const char ** extensions, unsigned int extensions_count, const char ** layers, unsigned int layerCount, const char * app_name, uint32_t app_ver, 
	const char * engine_name, uint32_t engine_ver, uint32_t api_version)
{
	VkInstance instance;

	VkApplicationInfo app_info = VkHelper::ApplicationInfo(
		app_name,                                                // Application name
		app_ver,                                                 // Application version
		engine_name,                                             // Engine name
		engine_ver,                                              // Engine version
		api_version                                              // Required API version
	);

	VkInstanceCreateInfo create_info = VkHelper::InstanceCreateInfo(
		app_info,                                                // Pointer to the application information created
		extensions,                                              // The raw data of the extensions to enable
		extensions_count,                                        // The amount of extensions we wish to enable
		layers,                                                  // The raw data of the layers to enable
		layerCount                                               // The amount of layers we wish to enable
	);

	VkResult result = vkCreateInstance(
		&create_info,                                            // Information to pass to the function
		NULL,                                                    // Memory allocation callback
		&instance                                                // The Vulkan instance to be initialized
	);

	// Was the vulkan instance created sucsessfully
	assert(result == VK_SUCCESS);

	return instance;
}

// Attach a debugger to the application to give us validation feedback.
// This is usefull as it tells us about any issues without application
VkDebugReportCallbackEXT VkHelper::CreateDebugger(const VkInstance& instance)
{
	// Get the function pointer for the debug callback function within VK
	PFN_vkCreateDebugReportCallbackEXT vkCreateDebugReportCallbackEXT =
		reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT>
		(vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT"));


	/*PFN_vkDebugReportMessageEXT vkDebugReportMessageEXT =
		reinterpret_cast<PFN_vkDebugReportMessageEXT>
		(vkGetInstanceProcAddr(instance, "vkDebugReportMessageEXT"));
	PFN_vkDestroyDebugReportCallbackEXT vkDestroyDebugReportCallbackEXT =
		reinterpret_cast<PFN_vkDestroyDebugReportCallbackEXT>
		(vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT"));*/


	// Define a CreateInfo for our new callback
	VkDebugReportCallbackCreateInfoEXT callbackCreateInfo;
	callbackCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;            // Callback Type
	callbackCreateInfo.pNext = nullptr;
	callbackCreateInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT |                            // What we wukk be notified about
		VK_DEBUG_REPORT_WARNING_BIT_EXT |                                                 //...
		VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;                                      //...
	callbackCreateInfo.pfnCallback = &MyDebugReportCallback;                              // Our function that will be called on a callback
	callbackCreateInfo.pUserData = nullptr;                                               // A custom data pointer that the user can define. Since we are calling a non member function
	                                                                                      // it may be usefull to pass a pointer instance of the engine or rendering libary, in this case
	                                                                                      // we dont need anything

	VkDebugReportCallbackEXT callback;

	// Create the new callback
	VkResult result = vkCreateDebugReportCallbackEXT(instance, &callbackCreateInfo, nullptr, &callback);

	// Was the vulkan callback created sucsessfully
	assert(result == VK_SUCCESS);

	return callback;
}

void VkHelper::DestroyDebugger(const VkInstance & instance, const VkDebugReportCallbackEXT & debugger)
{
	PFN_vkDestroyDebugReportCallbackEXT vkDestroyDebugReportCallbackEXT =
		reinterpret_cast<PFN_vkDestroyDebugReportCallbackEXT>
		(vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT"));

	// Destroy the debug callback
	vkDestroyDebugReportCallbackEXT(
		instance,
		debugger,
		nullptr
	);
}

bool VkHelper::HasRequiredExtentions(const VkPhysicalDevice & physical_device, const char ** required_extentions, const uint32_t & required_extention_count)
{
	// Get all the extentions on device
	// First we need to get the extention count
	uint32_t extension_count;
	vkEnumerateDeviceExtensionProperties(
		physical_device,                                       // Physical Device we are checking
		nullptr,                                               // 
		&extension_count,                                      // A return parameter to allow us to get the amount of extentions on the gpu
		nullptr                                                // A return pointer that sets a pointer array full of the extention names
															   // When set to nullptr, we are telling the system we want to get the device count
	);

	// Now get all the avaliable extentions on the device
	std::unique_ptr<VkExtensionProperties[]> available_extensions(new VkExtensionProperties[extension_count]());
	vkEnumerateDeviceExtensionProperties(
		physical_device,                                       // Physical Device we are checking
		nullptr,                                               // 
		&extension_count,                                      // A return parameter to allow us to get the amount of extentions on the gpu
		available_extensions.get()                             // A return pointer that sets a pointer array full of the extention names
	);

	// Loop through all wanted extentions to make sure they all exist
	for (int i = 0; i < required_extention_count; ++i)
	{
		bool extention_found = false;
		// Loop through for each avaliable device extention and atempt to find our required extention
		for (int j = 0; j < extension_count; ++j)
		{
			// Check to see if the layer matches
			if (strcmp(required_extentions[i], available_extensions[j].extensionName) == 0)
			{
				extention_found = true;
				break;
			}
		}
		// If we are missing the extention layer, report back
		if (!extention_found)
		{
			return false;
		}
	}
	return true;
}

bool VkHelper::GetQueueFamily(const VkPhysicalDevice & physical_device, VkQueueFlags required_queue_flags, uint32_t & queue_family_index, VkSurfaceKHR surface)
{
	// Fist we need to get the amount of queue families on the system
	uint32_t queue_family_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(
		physical_device,                                       // Physical Device we are checking
		&queue_family_count,                                   // A return parameter to allow us to get the amount of queue families on the gpu
		nullptr                                                // A return pointer that sets a pointer array full of the extention names
	);

	// Next get the queue families
	std::unique_ptr<VkQueueFamilyProperties[]> queue_families(new VkQueueFamilyProperties[queue_family_count]());
	vkGetPhysicalDeviceQueueFamilyProperties(
		physical_device,                                       // Physical Device we are checking
		&queue_family_count,                                   // A return parameter to allow us to get the amount of queue families on the gpu
		queue_families.get()                                   // A return pointer that sets a pointer array full of the extention names
															   // When set to nullptr, we are telling the system we want to get the queue family count
	);

	// Loop through all the queue families to find one that matches our criteria
	for (int i = 0; i < queue_family_count; ++i)
	{
		// Make sure there are queues in the queue family
		if (queue_families[i].queueCount > 0)
		{
			// Check if the queue family has the correct queues
			// Using bitwise AND check that the queue flags are set
			if ((queue_families[i].queueFlags & required_queue_flags) == required_queue_flags)
			{
				// If we are not needing to check for surface support, we just return the first found
				if (surface == VK_NULL_HANDLE)
				{
					queue_family_index = i;
					return true;
				}
				// If we need to check surface support we call vkGetPhysicalDeviceSurfaceSupportKHR
				VkBool32 present_support = false;
				vkGetPhysicalDeviceSurfaceSupportKHR(
					physical_device,
					i,
					surface,
					&present_support
				);
				if (present_support)
				{
					queue_family_index = i;
					return true;
				}
			}
		}
	}
	// Return a invalid state
	return false;
}

bool VkHelper::GetPhysicalDevice(const VkInstance& instance, VkPhysicalDevice & physical_device, VkPhysicalDeviceProperties & device_properties, uint32_t & queue_family_index,
	VkPhysicalDeviceFeatures& device_features, VkPhysicalDeviceMemoryProperties& device_mem_properties, const char ** physical_device_extentions, const unsigned int extention_count,
	VkQueueFlags required_queue_flags, VkSurfaceKHR surface)
{
	// Find all avaliable devices within the system
	// Get the device count
	uint32_t device_count = 0;
	vkEnumeratePhysicalDevices(
		instance,                                           // Refrence to our Vulkan Instance
		&device_count,                                      // A return parameter for the amount of system devices
		nullptr                                             // A pointer return parameter for all devices, if left nullptr, then we are saying, we just want the device count
	);

	// Get the Device Instances
	std::unique_ptr<VkPhysicalDevice[]> devices(new VkPhysicalDevice[device_count]());
	// Get the physical devices
	vkEnumeratePhysicalDevices(
		instance,                                           // Refrence to our Vulkan Instance
		&device_count,                                      // A return parameter for the amount of system devices
		devices.get()                                       // A pointer return parameter to get all devices
	);

	// Now we have all the physical devices that are inside the device, we need to find a suitable one for our needs
	for (int i = 0; i < device_count; i++)
	{
		// First we need to check that the device has all the required extentions
		if (HasRequiredExtentions(devices[i], physical_device_extentions, extention_count))
		{
			// Next each physical device has a list of queue families that controll the flow of commands to and from the CPU side of the system
			// to the GPU side. Each queue family contains a diffrent amount of Queues, some contain all of them, some may contain a mixed bag or
			// some may only contain 1.

			// The common queues we consern ourself with is the GraphicsQueue, but there is also compute (for general compute tasks), 
			// transfer (moving data between the devices) and sparse (for device memory managment).

			uint32_t queue_family = 0;
			if (GetQueueFamily(devices[i], required_queue_flags, queue_family, surface))
			{
				// Now we know the device is valid, there may be several valid devices within the system and we need a way of choosing between them
				// So we can get the properties of the device and find a way of prioritising them
				VkPhysicalDeviceProperties physical_device_properties;
				vkGetPhysicalDeviceProperties(
					devices[i],
					&physical_device_properties
				);
				// Find the features that are avaliable on the device
				VkPhysicalDeviceFeatures physical_device_features;
				vkGetPhysicalDeviceFeatures(
					devices[i],
					&physical_device_features
				);

				// Get all information about the devices memory
				VkPhysicalDeviceMemoryProperties physical_device_mem_properties;
				vkGetPhysicalDeviceMemoryProperties(
					devices[i],
					&physical_device_mem_properties
				);

				// First if the chosen device is null and the only GPU that reports back is a intergrated gpu then use it untill we find a deticated one
				if (physical_device == VK_NULL_HANDLE || physical_device != VK_NULL_HANDLE && physical_device_properties.deviceType == VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
				{
					physical_device = devices[i];
					queue_family_index = queue_family;
					device_properties = physical_device_properties;
					device_features = physical_device_features;
					device_mem_properties = physical_device_mem_properties;
				}
			}
		}
	}

	// Check to see if the device is valid
	return physical_device != VK_NULL_HANDLE;
}

VkDevice VkHelper::CreateDevice(const VkPhysicalDevice & physical_device, VkDeviceQueueCreateInfo * queue_infos, uint32_t queue_info_count, VkPhysicalDeviceFeatures & physical_device_features,
	const char** extensions, unsigned int extensions_count)
{
	// Create a device create info for the device
	VkDeviceCreateInfo device_create_info = VkHelper::DeviceCreateInfo(
		queue_infos,                                            // Queues we want to enable
		queue_info_count,                                       // How many queue instances
		physical_device_features,                               // The physical devices instances
		extensions,                                             // What device extentions do we want
		extensions_count                                        // How many extentions
	);
	VkDevice device = VK_NULL_HANDLE;
	// Finish off by creating the device itself
	VkResult result = vkCreateDevice(
		physical_device,
		&device_create_info,
		nullptr,
		&device
	);

	// Was the vulkan device created sucsessfully
	assert(result == VK_SUCCESS);

	return device;
}

VkCommandPool VkHelper::CreateCommandPool(const VkDevice & device, const uint32_t & queue_family, VkCommandPoolCreateFlags flags)
{
	VkCommandPoolCreateInfo pool_info = VkHelper::CommandPoolCreateInfo(
		queue_family,                                                    // What queue family we are wanting to use to send commands to the GPU
		flags
	);

	VkCommandPool command_pool = VK_NULL_HANDLE;
	// Create the command pool
	VkResult result = vkCreateCommandPool(
		device,
		&pool_info,
		nullptr,
		&command_pool
	);
	// Was the vulkan command pool created sucsessfully
	assert(result == VK_SUCCESS);
	return command_pool;
}

uint32_t VkHelper::FindMemoryType(const VkPhysicalDeviceMemoryProperties& physical_device_mem_properties,uint32_t type_filter, VkMemoryPropertyFlags properties)
{
	// Loop through all memory types on the gpu
	for (uint32_t i = 0; physical_device_mem_properties.memoryTypeCount; i++)
	{
		// if we find a memory type that matches our type filter and the type has the required properties
		if (type_filter & (1 << i) && (physical_device_mem_properties.memoryTypes[i].propertyFlags & properties) == properties)
		{
			return i;
		}
	}
	assert(0 && "No available memory properties");
	return -1;
}

bool VkHelper::CreateBuffer(const VkDevice& device, const VkPhysicalDeviceMemoryProperties& physical_device_mem_properties, VkBuffer & buffer, VkDeviceMemory & buffer_memory, VkDeviceSize size, VkBufferUsageFlags usage, VkSharingMode sharing_mode, VkMemoryPropertyFlags buffer_memory_properties)
{


	VkBufferCreateInfo buffer_info = VkHelper::BufferCreateInfo(
		size,                                                            // How big do we want to buffer to be
		usage,                                                           // What type of buffer do we want. Buffers can have multiple types, for example, uniform & vertex buffer.
																		 // for now we want to keep the buffer spetilised to one type as this will allow vulkan to optimize the data.
		sharing_mode                                                     // There are two modes, exclusive and concurrent. Defines if it can concurrently be used by multiple queue
																		 // families at the same time
	);


	// Next we create the buffer. Even though we told the buffer how much data we want to use with it, it dose not actualy create the data yet, this is because we can move data
	// between buffers at any time by rebinding them. You will see this process later when we first bind some data to this buffer
	VkResult buffer_result = vkCreateBuffer(
		device,                                                          // What device are we going to use to create the buffer
		&buffer_info,                                                    // What create info are we going to use to base the new buffer on
		nullptr,                                                         // ...
		&buffer                                                          // What buffer are we going to create to
	);

	// Make sure the buffer was created sucsessfully
	assert(buffer_result == VK_SUCCESS);


	// Find out what the memory requirments of the buffer are
	VkMemoryRequirements buffer_memory_requirements;
	vkGetBufferMemoryRequirements(
		device,                                                          // What device we are using to find the buffer requirments
		buffer,                                                          // What buffer we are getting the requirments for
		&buffer_memory_requirements                                      // Where are we wrighting the requirments too
	);


	// Next we need to find the type of memory required for this buffer
	uint32_t memory_type = FindMemoryType(
		physical_device_mem_properties,                                 // Properties describing the memory avaliable on the GPU
		buffer_memory_requirements.memoryTypeBits,                      // What memory type vulkan has told us we need
		buffer_memory_properties                                        // What properties do we rquire of our memory
	);


	VkMemoryAllocateInfo memory_allocate_info = VkHelper::MemroyAllocateInfo(
		buffer_memory_requirements.size,                                // How much memory we wish to allocate on the GPU
		memory_type                                                     // What tyoe if memory we want to allocate
	);

	// Now we have defines what type of memory we want to allocate and how much of it, we allocate it.
	VkResult memory_allocation_result = vkAllocateMemory(
		device,                                                         // What device we want to allocate the memory on
		&memory_allocate_info,                                          // The allocate info for memory allocation
		nullptr,                                                        // ...
		&buffer_memory                                                  // The output for the buffer memory
	);

	// Could we allocate the new memory
	assert(memory_allocation_result == VK_SUCCESS);

	// Now we have the buffer and the memory, we need to bind them together.
	// The bind buffer memory has a offset value, this value is used when we allocate a large amount of memory, but want to split it
	// between multiple buffers, we can tell it now how far from the memory start we want the buffer to start from
	VkResult bind_buffer_memory = vkBindBufferMemory(
		device,                                                         // What device the memory and buffer are on
		buffer,                                                         // What buffer we want to bind
		buffer_memory,                                                  // What memory we want to beind to the buffer
		0                                                               // The start offset from the beginning of the allocated memory
	);


	// Could we bind the new memory to the buffer
	assert(bind_buffer_memory == VK_SUCCESS);

	return true;
}

VkDescriptorPool VkHelper::CreateDescriptorPool(const VkDevice & device, VkDescriptorPoolSize* pool_sizes, uint32_t pool_sizes_count, uint32_t max_sets)
{
	VkDescriptorPool descriptor_pool = VK_NULL_HANDLE;

	VkDescriptorPoolCreateInfo create_info = VkHelper::DescriptorPoolCreateInfo(pool_sizes, pool_sizes_count, max_sets);

	VkResult create_descriptor_pool = vkCreateDescriptorPool(
		device,
		&create_info,
		nullptr,
		&descriptor_pool
	);

	assert(create_descriptor_pool == VK_SUCCESS);

	return descriptor_pool;
}

VkDescriptorSetLayout VkHelper::CreateDescriptorSetLayout(const VkDevice & device, VkDescriptorSetLayoutBinding * layout_bindings, uint32_t layout_binding_count)
{
	VkDescriptorSetLayout descriptor_set_layout = VK_NULL_HANDLE;

	VkDescriptorSetLayoutCreateInfo layout_info = VkHelper::DescriptorSetLayoutCreateInfo(layout_bindings, layout_binding_count);

	VkResult create_descriptor_set_layout = vkCreateDescriptorSetLayout(
		device,
		&layout_info,
		nullptr,
		&descriptor_set_layout
	);

	assert(create_descriptor_set_layout == VK_SUCCESS);

	return descriptor_set_layout;
}

VkDescriptorSet VkHelper::AllocateDescriptorSet(const VkDevice & device, const VkDescriptorPool & descriptor_pool, const VkDescriptorSetLayout & descriptor_set_layout, uint32_t count)
{
	VkDescriptorSet descriptor_set = VK_NULL_HANDLE;

	VkDescriptorSetAllocateInfo alloc_info = VkHelper::DescriptorSetAllocateInfo(descriptor_pool, descriptor_set_layout, count);

	VkResult create_descriptor_set = vkAllocateDescriptorSets(
		device,
		&alloc_info,
		&descriptor_set
	);

	assert(create_descriptor_set == VK_SUCCESS);

	return descriptor_set;
}

bool CheckSwapchainSupport(const VkPhysicalDevice & physical_device, const VkSurfaceKHR& surface, VkSurfaceCapabilitiesKHR& capabilities, VkSurfaceFormatKHR*& formats, uint32_t& format_count, VkPresentModeKHR*& modes, uint32_t& mode_count)
{
	VkResult physical_device_surface_capabilities_res = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
		physical_device,
		surface,
		&capabilities
	);

	assert(physical_device_surface_capabilities_res == VK_SUCCESS);


	// Get all formats
	vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count, nullptr);
	if (format_count == 0) return false;

	formats = new VkSurfaceFormatKHR[format_count];

	vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count, formats);

	// Get all the present modes
	vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &mode_count, nullptr);
	if (mode_count == 0) return false;

	modes = new VkPresentModeKHR[mode_count];

	vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &mode_count, modes);

	return true;
}

VkSurfaceFormatKHR ChooseSwapchainSurfaceFormat(const VkSurfaceFormatKHR* formats, const uint32_t& format_count)
{
	// If there is one format and that format is VK_FORMAT_UNDEFINED, the vulkan dose not mind what we set as a format
	if (format_count == 1 && formats[0].format == VK_FORMAT_UNDEFINED)
	{
		return{ VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
	}
	// Loop through formats and find the most appropriate one
	for (int i = 0; i < format_count; i++)
	{
		if (formats[i].format == VK_FORMAT_B8G8R8A8_UNORM && formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			return formats[i];
		}
	}
	// If we have got this far, its best just to go with the format specified
	if (format_count > 0)return formats[0];

	// Default and say we have a undefined format
	return{ VK_FORMAT_UNDEFINED };
}


VkPresentModeKHR ChooseSwapPresentMode(const VkPresentModeKHR* modes, const uint32_t& mode_count)
{
	// Loop through modes
	for (int i = 0; i < mode_count; i++)
	{
		// Loop for tipple buffering
		if (modes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			// Temporerply dissable everything but V-SYNC
			return modes[i];
		}
	}
	// Fallback format that is guaranteed to be available
	// This locks the rendering to V-Sync
	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR & capabilities, uint32_t window_width, uint32_t window_height)
{
	if (capabilities.currentExtent.width != UINT32_MAX)
	{
		return capabilities.currentExtent;
	}
	else
	{
		VkExtent2D extent = { window_width, window_height };
		// Make sure the swap buffer image size is not too big or too small
		if (extent.width > capabilities.maxImageExtent.width)extent.width = capabilities.maxImageExtent.width;
		if (extent.width < capabilities.minImageExtent.width)extent.width = capabilities.minImageExtent.width;
		if (extent.height > capabilities.maxImageExtent.width)extent.height = capabilities.maxImageExtent.height;
		if (extent.height < capabilities.minImageExtent.width)extent.height = capabilities.minImageExtent.height;
		return extent;
	}
}

VkSwapchainKHR VkHelper::CreateSwapchain(const VkPhysicalDevice & physical_device, const VkDevice & device, const VkSurfaceKHR& surface, VkSurfaceCapabilitiesKHR & surface_capabilities,
	VkSurfaceFormatKHR& surface_format, VkPresentModeKHR& present_mode, uint32_t window_width, uint32_t window_height, uint32_t& swapchain_image_count, std::unique_ptr<VkImage>& swapchain_images,
	std::unique_ptr<VkImageView>& swapchain_image_views)
{
	VkSwapchainKHR swapchain = VK_NULL_HANDLE;

	VkSurfaceFormatKHR* surface_formats = nullptr;
	uint32_t surface_format_count = 0;
	VkPresentModeKHR* present_modes = nullptr;
	uint32_t present_mode_count = 0;

	bool hasSupport = CheckSwapchainSupport(physical_device, surface, surface_capabilities, surface_formats, surface_format_count, present_modes, present_mode_count);

	assert(hasSupport);


	surface_format = ChooseSwapchainSurfaceFormat(surface_formats, surface_format_count);
	present_mode = ChooseSwapPresentMode(present_modes, present_mode_count);

	VkExtent2D extent = ChooseSwapExtent(surface_capabilities, window_width, window_height);

	// Make sure that the amount of images we are creating for the swapchain are in between the min and max
	uint32_t image_count = surface_capabilities.minImageCount + 1;
	if (surface_capabilities.maxImageCount > 0 && image_count > surface_capabilities.maxImageCount)
	{
		image_count = surface_capabilities.maxImageCount;
	}



	VkSwapchainCreateInfoKHR create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	create_info.surface = surface; // The surface we are drawing onto
	create_info.minImageCount = image_count; // Amount of swapchain images
	create_info.imageFormat = surface_format.format; // What format will the images be in
	create_info.imageColorSpace = surface_format.colorSpace; // What colors will the images use
	create_info.imageExtent = extent; // Image width and height
	create_info.imageArrayLayers = 1; // Keep at 1
	create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;// | VK_IMAGE_USAGE_STORAGE_BIT; // VK_IMAGE_USAGE_STORAGE_BIT used for raytracing
	create_info.presentMode = present_mode;
	create_info.clipped = VK_TRUE; // If we can't see a pixel, get rid of it
	create_info.oldSwapchain = VK_NULL_HANDLE; // We are not remaking the swapchain

	create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE; // image is owned by one queue family
	create_info.queueFamilyIndexCount = 0;
	create_info.pQueueFamilyIndices = nullptr;

	create_info.preTransform = surface_capabilities.currentTransform; // We don't want any transformations applied to the images
	create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; // Ignore the alpha channel so we can do blending
















	// If we support transfering data from a buffer to this image, then we can enable the feature on the swapchain image
	if ((surface_capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT) == VK_IMAGE_USAGE_TRANSFER_DST_BIT)
	{
		create_info.imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	}

	// If we support transfering data from the swapchain image to a buffer, then we can enable the feature on the swapchain image
	if ((surface_capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) == VK_IMAGE_USAGE_TRANSFER_SRC_BIT)
	{
		create_info.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	}

	// Create a new swapchain using the constructor structure
	VkResult create_swapchain_resut = vkCreateSwapchainKHR(
		device,
		&create_info,
		nullptr,
		&swapchain
	);

	// Validate the swapchain was created, do this at debug time
	assert(create_swapchain_resut == VK_SUCCESS);

	// Get the swapchain image count
	VkResult get_swapchain_images_sucsess = vkGetSwapchainImagesKHR(
		device,
		swapchain,
		&swapchain_image_count,
		nullptr
	);

	// Make sure the swapchain image count was returned correctly
	assert(get_swapchain_images_sucsess == VK_SUCCESS);

	swapchain_images = std::unique_ptr<VkImage>(new VkImage[swapchain_image_count]);

	// Get the refrence back to the swapchain images
	get_swapchain_images_sucsess = vkGetSwapchainImagesKHR(
		device,
		swapchain,
		&swapchain_image_count,
		swapchain_images.get()
	);

	// Make sure the swapchain images was returned correctly
	assert(get_swapchain_images_sucsess == VK_SUCCESS);


	swapchain_image_views = std::unique_ptr<VkImageView>(new VkImageView[swapchain_image_count]);

	// Loop through for the swapchain images and create new image views
	for (int i = 0; i < swapchain_image_count; i++)
	{
		// Define how the image view will be created
		VkImageViewCreateInfo image_view_create_info = {};
		image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		image_view_create_info.image = swapchain_images.get()[i];							// Give refrence to the swapchain made image
		image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;							// What tyoe if uange us ut
		image_view_create_info.format = surface_format.format;								// Whats the image format
		image_view_create_info.components.r = VK_COMPONENT_SWIZZLE_R;						// What order are the color channels extected
		image_view_create_info.components.g = VK_COMPONENT_SWIZZLE_G;						// ...
		image_view_create_info.components.b = VK_COMPONENT_SWIZZLE_B;						// ...
		image_view_create_info.components.a = VK_COMPONENT_SWIZZLE_A;						// ...
		image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		image_view_create_info.subresourceRange.baseMipLevel = 0;							// We wont need to use mip mapping for a swapchain image
		image_view_create_info.subresourceRange.levelCount = 1;								// How many mip levels does the image have
		image_view_create_info.subresourceRange.baseArrayLayer = 0;
		image_view_create_info.subresourceRange.layerCount = 1;

		// Create the new image view
		VkResult create_image_view_result = vkCreateImageView(
			device,
			&image_view_create_info,
			nullptr,
			&swapchain_image_views.get()[i]
		);

		assert(create_image_view_result == VK_SUCCESS);
	}

	return swapchain;
}

void VkHelper::CreateImage(const VkDevice& device, const VkPhysicalDeviceMemoryProperties& physical_device_mem_properties, uint32_t width, uint32_t height, VkFormat format,
	VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage & image, VkDeviceMemory & image_memory, VkImageLayout initialLayout)
{
	VkImageCreateInfo image_create_info = {};
	image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image_create_info.imageType = VK_IMAGE_TYPE_2D;
	image_create_info.extent.width = width;
	image_create_info.extent.height = height;
	image_create_info.extent.depth = 1;
	image_create_info.mipLevels = 1;
	image_create_info.arrayLayers = 1;
	image_create_info.format = format;
	image_create_info.tiling = tiling;
	image_create_info.initialLayout = initialLayout;
	image_create_info.usage = usage;
	image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
	image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;


	VkResult create_image_info_result = vkCreateImage(
		device,
		&image_create_info,
		nullptr,
		&image
	);
	assert(create_image_info_result == VK_SUCCESS);

	VkMemoryRequirements mem_requirements;
	vkGetImageMemoryRequirements(
		device,
		image,
		&mem_requirements
	); 
	
	uint32_t memory_type = FindMemoryType(
		physical_device_mem_properties,                                 // Properties describing the memory avaliable on the GPU
		mem_requirements.memoryTypeBits,                      // What memory type vulkan has told us we need
		properties                                        // What properties do we rquire of our memory
	);

	VkMemoryAllocateInfo memory_allocate_info = VkHelper::MemroyAllocateInfo(
		mem_requirements.size,                                          // How much memory we wish to allocate on the GPU
		memory_type                                                     // What tyoe if memory we want to allocate
	);
	VkResult allocate_result = vkAllocateMemory(
		device,
		&memory_allocate_info,
		nullptr,
		&image_memory
	);
	assert(allocate_result == VK_SUCCESS);

	VkResult bind_result = vkBindImageMemory(
		device,
		image,
		image_memory,
		0
	);
	assert(bind_result == VK_SUCCESS);
}



VkCommandBuffer VkHelper::BeginSingleTimeCommands(const VkDevice& device, const VkCommandPool& command_pool)
{
	// Create a single command buffer
	VkCommandBufferAllocateInfo alloc_info = VkHelper::CommandBufferAllocateInfo(
		command_pool,
		1
	);
	VkCommandBuffer command_buffer;
	// Allocate the command buffer from the GPU
	VkResult allocate_command_buffer_result = vkAllocateCommandBuffers(
		device,
		&alloc_info,
		&command_buffer
	);
	// Check to see if we allocated it okay
	assert(allocate_command_buffer_result == VK_SUCCESS);
	// Start the command buffer
	VkCommandBufferBeginInfo begin_info = VkHelper::CommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	VkResult begin_command_buffer = vkBeginCommandBuffer(
		command_buffer,
		&begin_info
	);
	assert(begin_command_buffer == VK_SUCCESS);
	return command_buffer;
}

void VkHelper::EndSingleTimeCommands(const VkDevice& device, const VkQueue& queue, VkCommandBuffer command_buffer, VkCommandPool command_pool)
{
	// End the command buffer
	vkEndCommandBuffer(command_buffer);
	// Submit the command buffer to the queue
	VkSubmitInfo submit_info = VkHelper::SubmitInfo(command_buffer);
	VkResult queue_submit_result = vkQueueSubmit(
		queue,
		1,
		&submit_info,
		VK_NULL_HANDLE
	);
	// Validate it submitted alright
	assert(queue_submit_result == VK_SUCCESS);
	// Wait for the GPU
	vkQueueWaitIdle(
		queue
	);
	// Release the command buffers
	vkFreeCommandBuffers(
		device,
		command_pool,
		1,
		&command_buffer
	);
}


void VkHelper::TransitionImageLayout(const VkDevice& device, const VkQueue& queue, VkCommandPool command_pool, VkImage image, VkFormat format, VkImageLayout old_layout, VkImageLayout new_layout, VkImageSubresourceRange subresourceRange)
{
	// Create a new single time command
	VkCommandBuffer command_buffer = BeginSingleTimeCommands(device, command_pool);

	// Define how we will convert between the image layouts
	VkImageMemoryBarrier barrier = VkHelper::ImageMemoryBarrier(image, format, old_layout, new_layout);

	barrier.subresourceRange = subresourceRange;
	// Submit the barrier update
	vkCmdPipelineBarrier(
		command_buffer,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		0,
		0,
		nullptr,
		0,
		nullptr,
		1,
		&barrier
	);
	// Submit the command to the GPU
	EndSingleTimeCommands(device, queue, command_buffer, command_pool);
}

void VkHelper::CreateAttachmentImages(const VkDevice& device, const VkQueue& queue, uint32_t width, uint32_t height, const VkPhysicalDeviceMemoryProperties& physical_device_mem_properties, const VkPhysicalDeviceFeatures& physical_device_features,
	const VkPhysicalDeviceProperties& physical_device_properties, const VkCommandPool& command_pool, VkFormat format, VkImageUsageFlags usage, VkHelper::FrameBufferAttachment & attachment)
{

	attachment.format = format;

	VkImageAspectFlags aspectMask = 0;
	VkImageLayout imageLayout;

	if (usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
	{
		aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	}
	if (usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
	{
		aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	}
	// Create a new image for the subpass attachment
	VkHelper::CreateImage(
		device,
		physical_device_mem_properties,
		width,
		height,
		format,
		VK_IMAGE_TILING_OPTIMAL,
		usage | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		attachment.image,
		attachment.memory,
		VK_IMAGE_LAYOUT_UNDEFINED
	);


	{
		// Create image view
		VkImageViewCreateInfo create_info = {};
		create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		create_info.image = attachment.image;
		create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		create_info.format = format;
		create_info.subresourceRange = {};
		create_info.subresourceRange.aspectMask = aspectMask;
		create_info.subresourceRange.baseMipLevel = 0;
		create_info.subresourceRange.levelCount = 1;
		create_info.subresourceRange.baseArrayLayer = 0;
		create_info.subresourceRange.layerCount = 1;
		VkResult create_image_view_result = vkCreateImageView(
			device,
			&create_info,
			nullptr,
			&attachment.view
		);
		assert(create_image_view_result == VK_SUCCESS);
	}

	{ // Define image sampler

		VkSamplerCreateInfo sampler_info{};
		sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		sampler_info.maxAnisotropy = 1.0f;
		sampler_info.magFilter = VK_FILTER_LINEAR;
		sampler_info.minFilter = VK_FILTER_LINEAR;
		sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		sampler_info.mipLodBias = 0.0f;
		sampler_info.compareOp = VK_COMPARE_OP_NEVER;
		sampler_info.minLod = 0.0f;
		// Set max level-of-detail to mip level count of the texture
		sampler_info.maxLod = (float)1;
		// Enable anisotropic filtering
		// This feature is optional, so we must check if it's supported on the device
		if (physical_device_features.samplerAnisotropy)
		{
			// Use max. level of anisotropy for this example
			sampler_info.maxAnisotropy = physical_device_properties.limits.maxSamplerAnisotropy;
			sampler_info.anisotropyEnable = VK_TRUE;
		}
		else
		{
			// The device does not support anisotropic filtering
			sampler_info.maxAnisotropy = 1.0;
			sampler_info.anisotropyEnable = VK_FALSE;
		}
		sampler_info.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		VkResult create_sampler_result = vkCreateSampler(
			device,
			&sampler_info,
			nullptr,
			&attachment.sampler
		);
		assert(create_sampler_result == VK_SUCCESS);
	}
	// If the image is used for color. switch the images layout
	if (usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
		TransitionImageLayout(device, queue, command_pool, attachment.image, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });
}

VkFormat VkHelper::FindSupportedFormat(const VkPhysicalDevice & physical_device, const VkFormat * candidate_formats, const uint32_t candidate_format_count, VkImageTiling tiling, VkFormatFeatureFlags features)
{
	// Loop through the formats we are considering
	for (int i = 0; i < candidate_format_count; i++)
	{
		VkFormatProperties props;
		// Read the properties for the format we are considering
		vkGetPhysicalDeviceFormatProperties(physical_device, candidate_formats[i], &props);
		// Do we have the required features for linear tiling
		if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
		{
			return candidate_formats[i];
		}
		// Do we have the features for optimal tiling
		else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
		{
			return candidate_formats[i];
		}
	}
	assert(0 && "All formats are not supported");
	return VK_FORMAT_UNDEFINED;
}


VkRenderPass VkHelper::CreateRenderPass(const VkPhysicalDevice & physical_device, const VkDevice& device, VkFormat present_format, VkFormat image_color_format, const uint32_t& swapchain_image_count,
	const VkPhysicalDeviceMemoryProperties& physical_device_mem_properties, const VkPhysicalDeviceFeatures& physical_device_features, const VkPhysicalDeviceProperties& physical_device_properties,
	const VkCommandPool& command_pool, const VkQueue& queue, uint32_t width, uint32_t height, std::unique_ptr<VkFramebuffer>& framebuffers, std::unique_ptr<VulkanAttachments>& framebuffer_attachments,
	std::unique_ptr<VkImageView>& swapchain_image_views)
{
	VkRenderPass renderpass = VK_NULL_HANDLE;

	// How many formats are we considering for the depth image?
	const uint32_t candidate_format_count = 3;
	// What formats are we considering for the depth image
	// Most modern day GPU's support between two and three of these, VK_FORMAT_D32_SFLOAT is out prefered choice, but the other two will surfice
	const VkFormat candidate_formats[candidate_format_count] = {
		VK_FORMAT_D32_SFLOAT,
		VK_FORMAT_D32_SFLOAT_S8_UINT,
		VK_FORMAT_D24_UNORM_S8_UINT
	};

	// Out of the formats we defines, which one is supported by the gpu
	VkFormat depth_image_format = FindSupportedFormat(
		physical_device,
		candidate_formats,
		candidate_format_count,
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
	);


	VkAttachmentDescription present_attachment = {};
	VkAttachmentDescription color_attachment = {};
	VkAttachmentDescription depth_attachment = {};

	{	// Define how the present attachment will handle its resources
		present_attachment.format = present_format;
		present_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
		present_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;				// Each subpass (or frame as we only have 1 subpass) we want to clear the last present
		present_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;				// What do we want to do with the data after the subpass, we want to store it away
		present_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;		// Dont use stencil currently
		present_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;	// Dont use stencil currently 
		present_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;			// The layout the attachment image subresource, in out case its undefined
		present_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;		// Upon finishing what format do we want, since we are presenting, we need PRESENNT_KHR
	}

	{
		color_attachment.format = image_color_format;
		color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
		color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;					// Clear the color each subpass
		color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;			// After the subpass we spesify we dont care what happens to the data
		color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;		// Dont use stencil currently 
		color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;		// Dont use stencil currently 
		color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;				// The layout the attachment image subresource, in out case its undefined
		color_attachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;// The result will be a color attachment
	}

	{
		depth_attachment.format = depth_image_format;
		depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
		depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;							// Clear the depth each subpass
		depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;					// After the subpass we spesify we dont care what happens to the data
		depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;				// Dont use stencil currently 
		depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;				// Dont use stencil currently 
		depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;						// The layout the attachment image subresource, in out case its undefined
		depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;// The result will be a depth attachment
	}

	// How many attachment descriptions will we have
	const uint32_t attachment_description_count = 3;
	// Put the descriptions into a single array
	VkAttachmentDescription attachment_descriptions[attachment_description_count] = {
		present_attachment,
		color_attachment,
		depth_attachment
	};

	// Define the layout of each attachments
	VkAttachmentReference present_attachment_refrence = {};
	present_attachment_refrence.attachment = 0;
	present_attachment_refrence.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // Present color

	VkAttachmentReference color_attachment_refrence = {};
	color_attachment_refrence.attachment = 1;
	color_attachment_refrence.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;	// Color

	VkAttachmentReference depth_attachment_refrence = {};
	depth_attachment_refrence.attachment = 2;
	depth_attachment_refrence.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL; // Depth


	// How many color attachments we have
	const uint32_t color_attachment_refrence_count = 2;
	// Place all the color attachments into a single array
	VkAttachmentReference color_attachment_refrences[color_attachment_refrence_count] = {
		present_attachment_refrence,
		color_attachment_refrence
	};

	// In each subpass what attachment refrences we we use
	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = color_attachment_refrence_count;
	subpass.pColorAttachments = color_attachment_refrences;
	subpass.pDepthStencilAttachment = &depth_attachment_refrence;

	// What dependancys dose the single subpass we are using have
	VkSubpassDependency subpass_dependency = {};
	subpass_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;															// Defines what was the past sub pass we were in
	subpass_dependency.dstSubpass = 0;																				// Deifne the next subpass
	subpass_dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;								// What is the incoming attachment
	subpass_dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;								// Whats the outgoing attachment
	subpass_dependency.srcAccessMask = 0;																			// Source access mask
	subpass_dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;	// Destination access mask
	subpass_dependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;


	// Define the final create render pass structure
	VkRenderPassCreateInfo render_pass_create_info = {};
	render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	render_pass_create_info.attachmentCount = attachment_description_count;		// How many descriptions do we have
	render_pass_create_info.pAttachments = attachment_descriptions;				// Pointer to descriptions
	render_pass_create_info.subpassCount = 1;									// How many subpasses we have
	render_pass_create_info.pSubpasses = &subpass;								// Pointer back to the subpass definitions
	render_pass_create_info.dependencyCount = 1;								// How many dependancys are there for the subpass
	render_pass_create_info.pDependencies = &subpass_dependency;				// Pointer back to the dependancys

	// Create the render pass with the defined properties
	VkResult create_render_pass_result = vkCreateRenderPass(
		device,
		&render_pass_create_info,
		nullptr,
		&renderpass
	);

	// Validate the render pass creation
	assert(create_render_pass_result == VK_SUCCESS);

	// Create a framebuffer for each swapchain
	framebuffers = std::unique_ptr<VkFramebuffer>(new VkFramebuffer[swapchain_image_count]);
	framebuffer_attachments = std::unique_ptr<VulkanAttachments>(new VulkanAttachments[swapchain_image_count]);

	// Loop through for all the new framebuffers
	for (uint32_t i = 0; i < swapchain_image_count; i++)
	{
		// Create the images and make it to that they are readable as color attachments and as image samplers
		CreateAttachmentImages(device, queue, width, height, physical_device_mem_properties, physical_device_features, physical_device_properties, command_pool, image_color_format,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, framebuffer_attachments.get()[i].color);

		CreateAttachmentImages(device, queue, width, height, physical_device_mem_properties, physical_device_features, physical_device_properties, command_pool, depth_image_format,
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, framebuffer_attachments.get()[i].depth);

		// Define the attachment count
		const uint32_t attachment_count = 3;
		// Create a array containing all the image views rquired
		VkImageView attachments[attachment_count] = {
			swapchain_image_views.get()[i],
			framebuffer_attachments.get()[i].color.view,
			framebuffer_attachments.get()[i].depth.view,
		};

		// Frame buffer create info
		VkFramebufferCreateInfo framebuffer_info = {};
		framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebuffer_info.renderPass = renderpass;
		framebuffer_info.attachmentCount = attachment_count;
		framebuffer_info.pAttachments = attachments;
		framebuffer_info.width = width;
		framebuffer_info.height = height;
		framebuffer_info.layers = 1;

		// Create the framebuffers
		VkResult create_frame_buffer_result = vkCreateFramebuffer(
			device,
			&framebuffer_info,
			nullptr,
			&framebuffers.get()[i]
		);
		// Validate the framebuffer
		assert(create_render_pass_result == VK_SUCCESS);
	}

	return renderpass;
}

void VkHelper::ReadShaderFile(const char * filename, char *& data, unsigned int & size)
{
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	if (!file.is_open())
	{
		throw std::runtime_error("failed to open file!");
	}
	size = file.tellg();
	data = new char[size];
	file.seekg(0);
	file.read(data, size);
	file.close();
}

VkPipeline VkHelper::CreateGraphicsPipeline(const VkPhysicalDevice & physical_device, const VkDevice& device, const VkRenderPass& renderpass, VkPipelineLayout& graphics_pipeline_layout,
	uint32_t shader_count, const char* shader_path[], VkShaderStageFlagBits* shader_stages_bits, VkShaderModule* shader_modules,
	uint32_t descriptor_set_layout_count, const VkDescriptorSetLayout* descriptor_set_layout,
	uint32_t vertex_input_attribute_description_count, const VkVertexInputAttributeDescription* vertex_input_attribute_descriptions,
	uint32_t vertex_input_binding_description_count, const VkVertexInputBindingDescription* vertex_input_binding_descriptions,
	uint32_t dynamic_state_count, VkDynamicState* dynamic_states,
	VkPrimitiveTopology topology, VkPolygonMode polygon_mode , float line_width,
	VkCullModeFlags cull_mode, VkBool32 depth_write_enable, VkBool32 depth_test_enable)
{
	VkPipeline graphics_pipeline = VK_NULL_HANDLE;

	// Create a peipeline layout, a pipeline layout defines what descriptors to accept
	VkPipelineLayoutCreateInfo pipeline_layout_info = {};
	pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipeline_layout_info.setLayoutCount = descriptor_set_layout_count;
	pipeline_layout_info.pSetLayouts = descriptor_set_layout;
	pipeline_layout_info.pushConstantRangeCount = 0;
	pipeline_layout_info.pPushConstantRanges = 0;

	// Create the pipeline layout
	VkResult create_pipeline_layout_result = vkCreatePipelineLayout(
		device,
		&pipeline_layout_info,
		nullptr,
		&graphics_pipeline_layout
	);
	// Was the pipeline created
	assert(create_pipeline_layout_result == VK_SUCCESS);

	std::unique_ptr<VkPipelineShaderStageCreateInfo> shader_stages =
		std::unique_ptr<VkPipelineShaderStageCreateInfo>(new VkPipelineShaderStageCreateInfo[shader_count]);


	for (uint32_t i = 0; i < shader_count; i++)
	{
		char* shader_data = nullptr;
		unsigned int shader_size = 0;

		// Load the shader from file
		VkHelper::ReadShaderFile(
			shader_path[i],
			shader_data,
			shader_size
		);

		// Create the shader module
		VkShaderModuleCreateInfo shader_module_create_info = {};
		shader_module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		shader_module_create_info.codeSize = shader_size;
		shader_module_create_info.pCode = reinterpret_cast<const uint32_t*>(shader_data);


		// Create the shader
		VkResult create_shader_module = vkCreateShaderModule(
			device,
			&shader_module_create_info,
			nullptr,
			&shader_modules[i]
		);
		// Validate the shader module
		assert(create_shader_module == VK_SUCCESS);


		shader_stages.get()[i] = {};
		shader_stages.get()[i].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shader_stages.get()[i].pName = "main";							 		// In the shader whats the entry function name
		shader_stages.get()[i].stage = shader_stages_bits[i];			 		// Define what type of shader we are making
		shader_stages.get()[i].module = shader_modules[i];


		delete[] shader_data;
	}


	// Create the vertex input state create info to store all data relating to the vertex input
	VkPipelineVertexInputStateCreateInfo vertex_input_info = {};
	vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertex_input_info.vertexBindingDescriptionCount = vertex_input_binding_description_count;
	vertex_input_info.vertexAttributeDescriptionCount = vertex_input_attribute_description_count;
	vertex_input_info.pVertexBindingDescriptions = vertex_input_binding_descriptions;
	vertex_input_info.pVertexAttributeDescriptions = vertex_input_attribute_descriptions;


	// Define what type of data we will be processing and how it will be stitched together. in this instance, its a list of triangles
	VkPipelineInputAssemblyStateCreateInfo input_assembly = {};
	input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_assembly.topology = topology;
	input_assembly.primitiveRestartEnable = VK_FALSE;


	// Define that the pipeline will have one viewport and one scissor
	VkPipelineViewportStateCreateInfo viewport_state = {};
	viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewport_state.viewportCount = 1;
	viewport_state.scissorCount = 1;


	// Define various rasterizeer settings
	VkPipelineRasterizationStateCreateInfo rasterizer = {};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = polygon_mode;										// Will the rasterizer break up the triangles into pixels
	rasterizer.lineWidth = line_width;											// If lines are drawn (Not VK_POLYGON_MODE_FILL), how wide will they be
	rasterizer.cullMode = cull_mode;											// Will any culling be preformed on the model
	rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;								// What order is the vertex data coming in, this is to help determan culling
	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f;
	rasterizer.depthBiasClamp = 0.0f;
	rasterizer.depthBiasSlopeFactor = 0.0f;


	// Here we define information for multisampling, in this case we do not need multisampling and provide defaults
	VkPipelineMultisampleStateCreateInfo multisampling = {};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.minSampleShading = 1.0f;
	multisampling.pSampleMask = nullptr;
	multisampling.alphaToCoverageEnable = VK_FALSE;
	multisampling.alphaToOneEnable = VK_FALSE;


	// Define how the depth buffer will be used
	VkPipelineDepthStencilStateCreateInfo depth_stencil = {};
	depth_stencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depth_stencil.depthTestEnable = depth_test_enable;									// Do we want to test for depth
	depth_stencil.depthWriteEnable = depth_write_enable;								// Do we write to the depth buffer
	depth_stencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;							// Wright only if its closer to the screen
	depth_stencil.depthBoundsTestEnable = VK_FALSE;
	depth_stencil.stencilTestEnable = VK_FALSE;


	// Define how color blending will happen between attachments
	VkPipelineColorBlendAttachmentState color_blend_attachment = {};
	color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	color_blend_attachment.blendEnable = VK_TRUE;
	color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
	color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;


	// Define how much color blending will happen between draws, in this case we want none
	VkPipelineColorBlendStateCreateInfo color_blending = {};
	color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	color_blending.logicOpEnable = VK_FALSE;
	color_blending.logicOp = VK_LOGIC_OP_COPY;
	color_blending.attachmentCount = 1;
	color_blending.pAttachments = &color_blend_attachment;
	color_blending.blendConstants[0] = 0.0f;
	color_blending.blendConstants[1] = 0.0f;
	color_blending.blendConstants[2] = 0.0f;
	color_blending.blendConstants[3] = 0.0f;


	// Define the dynamic states
	VkPipelineDynamicStateCreateInfo dynamic_state = {};
	dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamic_state.pDynamicStates = dynamic_states;								// Pointer to the states
	dynamic_state.dynamicStateCount = dynamic_state_count;						// How many states do we have
	dynamic_state.flags = 0;


	// Create the entire pipeline create info structure
	VkGraphicsPipelineCreateInfo pipeline_info = {};
	pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipeline_info.stageCount = shader_count;
	pipeline_info.pStages = shader_stages.get();
	pipeline_info.pVertexInputState = &vertex_input_info;
	pipeline_info.pInputAssemblyState = &input_assembly;
	pipeline_info.pViewportState = &viewport_state;
	pipeline_info.pRasterizationState = &rasterizer;
	pipeline_info.pMultisampleState = &multisampling;
	pipeline_info.pDepthStencilState = &depth_stencil;
	pipeline_info.pColorBlendState = &color_blending;
	pipeline_info.pDynamicState = &dynamic_state;
	pipeline_info.layout = graphics_pipeline_layout;
	pipeline_info.renderPass = renderpass;
	pipeline_info.subpass = 0;
	pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
	pipeline_info.basePipelineIndex = -1;
	pipeline_info.flags = VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT;


	// Create the pipeline
	VkResult create_graphics_pipeline_result = vkCreateGraphicsPipelines(
		device,
		VK_NULL_HANDLE,
		1,
		&pipeline_info,
		nullptr,
		&graphics_pipeline
	);
	// Was the pipeline created okay
	assert(create_graphics_pipeline_result == VK_SUCCESS);
	return graphics_pipeline;
}

void VkHelper::CreateImageSampler(const VkDevice& device, const VkImage& image, VkFormat format, VkImageView& imageView, VkSampler& sampler)
{
	VkSamplerCreateInfo sampler_info = SamplerCreateInfo();
	vkCreateSampler(
		device,
		&sampler_info,
		nullptr,
		&sampler
	);

	VkImageViewCreateInfo view_info = ImageViewCreate(image, format, VK_IMAGE_ASPECT_COLOR_BIT);
	view_info.subresourceRange.levelCount = 1;

	vkCreateImageView(
		device,
		&view_info,
		nullptr,
		&imageView
	);
}

void VkHelper::SetImageLayout(VkCommandBuffer cmdbuffer, VkImage image, VkImageLayout oldImageLayout, VkImageLayout newImageLayout, VkImageSubresourceRange subresourceRange)
{
	VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;

	// Create an image barrier object
	VkImageMemoryBarrier imageMemoryBarrier{};
	imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

	imageMemoryBarrier.oldLayout = oldImageLayout;
	imageMemoryBarrier.newLayout = newImageLayout;
	imageMemoryBarrier.image = image;
	imageMemoryBarrier.subresourceRange = subresourceRange;

	// Source layouts (old)
	// Source access mask controls actions that have to be finished on the old layout
	// before it will be transitioned to the new layout
	/*switch (oldImageLayout)
	{
	case VK_IMAGE_LAYOUT_UNDEFINED:
		// Image layout is undefined (or does not matter)
		// Only valid as initial layout
		// No flags required, listed only for completeness
		imageMemoryBarrier.srcAccessMask = 0;
		break;

	case VK_IMAGE_LAYOUT_PREINITIALIZED:
		// Image is preinitialized
		// Only valid as initial layout for linear images, preserves memory contents
		// Make sure host writes have been finished
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
		break;

	case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
		// Image is a color attachment
		// Make sure any writes to the color buffer have been finished
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		break;

	case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
		// Image is a depth/stencil attachment
		// Make sure any writes to the depth/stencil buffer have been finished
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		break;

	case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
		// Image is a transfer source
		// Make sure any reads from the image have been finished
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		break;

	case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
		// Image is a transfer destination
		// Make sure any writes to the image have been finished
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		break;

	case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
		// Image is read by a shader
		// Make sure any shader reads from the image have been finished
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		break;
	default:
		// Other source layouts aren't handled (yet)
		break;
	}

	// Target layouts (new)
	// Destination access mask controls the dependency for the new image layout
	switch (newImageLayout)
	{
	case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
		// Image will be used as a transfer destination
		// Make sure any writes to the image have been finished
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		break;

	case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
		// Image will be used as a transfer source
		// Make sure any reads from the image have been finished
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		break;

	case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
		// Image will be used as a color attachment
		// Make sure any writes to the color buffer have been finished
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		break;

	case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
		// Image layout will be used as a depth/stencil attachment
		// Make sure any writes to depth/stencil buffer have been finished
		imageMemoryBarrier.dstAccessMask = imageMemoryBarrier.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		break;

	case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
		// Image will be read in a shader (sampler, input attachment)
		// Make sure any writes to the image have been finished
		if (imageMemoryBarrier.srcAccessMask == 0)
		{
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
		}
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		break;
	default:
		// Other source layouts aren't handled (yet)
		break;
	}*/


	if (oldImageLayout == VK_IMAGE_LAYOUT_UNDEFINED && newImageLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		imageMemoryBarrier.srcAccessMask = 0;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldImageLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newImageLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
		dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else
	{
		//throw std::invalid_argument("unsupported layout transition!");
	}





	// Put barrier inside setup command buffer
	vkCmdPipelineBarrier(
		cmdbuffer,
		srcStageMask,
		dstStageMask,
		0,
		0, nullptr,
		0, nullptr,
		1, &imageMemoryBarrier);
}

void VkHelper::CreateFence(const VkDevice& device, std::unique_ptr<VkFence>& fences, unsigned int count)
{
	// Create a new pointer array for the fences
	fences = std::unique_ptr<VkFence>(new VkFence[count]);
	// Loop through for the amount of swapchain images
	for (unsigned int i = 0; i < count; i++)
	{
		// Define the fence create info
		VkFenceCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
		// Create the new fence
		VkResult create_fence_result = vkCreateFence(
			device,
			&info,
			nullptr,
			&fences.get()[i]
		);
		// Validate the fence
		assert(create_fence_result == VK_SUCCESS);
	}
}

void VkHelper::CreateVkSemaphore(const VkDevice& device, VkSemaphore& semaphore)
{
	VkSemaphoreCreateInfo semaphore_create_info = {};
	semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	// Create new signal semaphore
	VkResult create_semaphore_result = vkCreateSemaphore(device,
		&semaphore_create_info,
		nullptr,
		&semaphore
	);
	// Was the semaphore created
	assert(create_semaphore_result == VK_SUCCESS);
}
