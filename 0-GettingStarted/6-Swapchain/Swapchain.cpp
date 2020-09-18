
#define VK_USE_PLATFORM_WIN32_KHR
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <memory>
#include <stdexcept>

#include <SDL.h>
#include <SDL_syswm.h>
#include <vulkan/vulkan.h>
#include <VkInitializers.hpp>
#include <VkCore.hpp>

VkInstance instance;
VkDebugReportCallbackEXT debugger;
VkPhysicalDevice physical_device = VK_NULL_HANDLE;


uint32_t physical_devices_queue_family = 0;
VkPhysicalDeviceProperties physical_device_properties;
VkPhysicalDeviceFeatures physical_device_features;
VkPhysicalDeviceMemoryProperties physical_device_mem_properties;

VkDevice device = VK_NULL_HANDLE;
VkCommandPool command_pool;

const unsigned int buffer_array_length = 64;
// Create a array of data to store on the GPU
unsigned int example_input_buffer_data[buffer_array_length];

// Next we need to define how many bytes of data we want to reserve on the GPU
VkDeviceSize buffer_size = sizeof(unsigned int) * buffer_array_length;
// Create a storage variable for the buffer and buffer memory
VkBuffer buffer = VK_NULL_HANDLE;
VkDeviceMemory buffer_memory = VK_NULL_HANDLE;
// Raw pointer that will point to GPU memory
void* mapped_buffer_memory = nullptr;


VkDescriptorPool descriptor_pool;
VkDescriptorSetLayout descriptor_set_layout;
VkDescriptorSet descriptor_set;
VkWriteDescriptorSet descriptor_write_set;

SDL_Window* window;
SDL_SysWMinfo window_info;
VkSurfaceKHR surface;
uint32_t window_width;
uint32_t window_height;

VkSurfaceFormatKHR surface_format;
VkPresentModeKHR present_mode;

VkSwapchainKHR swap_chain;
std::unique_ptr<VkImage> swapchain_images;
uint32_t swapchain_image_count;
std::unique_ptr<VkImageView> swapchain_image_views;

// Create a new sdl window
void WindowSetup(const char* title, int width, int height)
{
	// Create window context with the surface usable by vulkan
	window = SDL_CreateWindow(
		title,
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		width, height,
		SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE
	);
	SDL_ShowWindow(window);

	window_width = width;
	window_height = height;

	SDL_VERSION(&window_info.version);
	// Verify the window was created correctly
	bool sucsess = SDL_GetWindowWMInfo(window, &window_info);
	assert(sucsess && "Error, unable to get window info");
}

// Check for window updates and process them
void PollWindow()
{
	// Currently the windowing system is very very basic and we do not need to process any events
	// Poll Window
	SDL_Event event;
	bool rebuild = false;
	while (SDL_PollEvent(&event) > 0)
	{
		switch (event.type)
		{
		case SDL_QUIT:

			break;
		}
	}
}

// Destroy current window context
void DestroyWindow()
{
	SDL_DestroyWindow(window);
}

// Create windows surface for sdl to interface with
void CreateSurface()
{
	auto CreateWin32SurfaceKHR = (PFN_vkCreateWin32SurfaceKHR)vkGetInstanceProcAddr(instance, "vkCreateWin32SurfaceKHR");

	VkWin32SurfaceCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	createInfo.hwnd = window_info.info.win.window;
	createInfo.hinstance = GetModuleHandle(nullptr);

	if (!CreateWin32SurfaceKHR || CreateWin32SurfaceKHR(instance, &createInfo, nullptr, &surface) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create window surface!");
	}
}

// Everything within the Setup is from previous tuturials
// Setup
// - Instance
// - Debugger
// - Physical Device
// - Device
// - Command Pool
// - Buffer
// - Descriptors
void Setup()
{
	// Define what Layers and Extentions we require
	const uint32_t extention_count = 3;
	const char *instance_extensions[extention_count] = { "VK_EXT_debug_report" ,VK_KHR_SURFACE_EXTENSION_NAME,VK_KHR_WIN32_SURFACE_EXTENSION_NAME };
	const uint32_t layer_count = 1;
	const char *instance_layers[layer_count] = { "VK_LAYER_LUNARG_standard_validation" };

	// Check to see if we have the layer requirments
	assert(VkHelper::CheckLayersSupport(instance_layers, 1) && "Unsupported Layers Found");

	// Create the Vulkan Instance
	instance = VkHelper::CreateInstance(
		instance_extensions, extention_count,
		instance_layers, layer_count,
		"Swapchain", VK_MAKE_VERSION(1, 0, 0),
		"Vulkan", VK_MAKE_VERSION(1, 0, 0),
		VK_MAKE_VERSION(1, 1, 108));

	// Attach a debugger to the application to give us validation feedback.
	// This is usefull as it tells us about any issues without application
	debugger = VkHelper::CreateDebugger(instance);

	// Define what Device Extentions we require
	const uint32_t physical_device_extention_count = 1;
	// Note that this extention list is diffrent from the instance on as we are telling the system what device settings we need.
	const char *physical_device_extensions[physical_device_extention_count] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };



	// The surface creation is added here as it needs to happen before the physical device creation and after we have said to the instance that we need a surface
	// The surface is the refrence back to the OS window
	CreateSurface();



	// Find a physical device for us to use
	bool foundPhysicalDevice = VkHelper::GetPhysicalDevice(
		instance, 
		physical_device,                                       // Return of physical device instance
		physical_device_properties,                            // Physical device properties
		physical_devices_queue_family,                         // Physical device queue family
		physical_device_features,                              // Physical device features
		physical_device_mem_properties,                        // Physical device memory properties
		physical_device_extensions,                            // What extentions out device needs to have
		physical_device_extention_count,                       // Extention count
		VK_QUEUE_GRAPHICS_BIT,                                 // What queues we need to be avaliable
		surface                                                // Pass the instance to the OS monitor
	);

	// Make sure we found a physical device
	assert(foundPhysicalDevice);

	// Define how many queues we will need in our project, for now, we will just create a single queue
	static const float queue_priority = 1.0f;
	VkDeviceQueueCreateInfo queue_create_info = VkHelper::DeviceQueueCreateInfo(
		&queue_priority,
		1,
		physical_devices_queue_family
	);


	// Now we have the physical device, create the device instance
	device = VkHelper::CreateDevice(                           
		physical_device,                                       // The physical device we are basic the device from
		&queue_create_info,                                    // A pointer array, pointing to a list of queues we want to make
		1,                                                     // How many queues are in the list
		physical_device_features,                              // What features do you want enabled on the device
		physical_device_extensions,                            // What extentions do you want on the device
		physical_device_extention_count                        // How many extentions are there
	);


	command_pool = VkHelper::CreateCommandPool(
		device,
		physical_devices_queue_family,                         // What queue family we are wanting to use to send commands to the GPU
		VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT        // Allows any commands we create, the ability to be reset. This is helpfull as we wont need to
	);                                                         // keep allocating new commands, we can reuse them




	// Initalize the array that we will pass to the GPU
	for (unsigned int i = 0; i < buffer_array_length; i++)
		example_input_buffer_data[i] = i; // Init the array with some basic data

	bool buffer_created = VkHelper::CreateBuffer(
		device,                                                          // What device are we going to use to create the buffer
		physical_device_mem_properties,                                  // What memory properties are avaliable on the device
		buffer,                                                          // What buffer are we going to be creating
		buffer_memory,                                                   // The output for the buffer memory
		buffer_size,                                                     // How much memory we wish to allocate on the GPU
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,                              // What type of buffer do we want. Buffers can have multiple types, for example, uniform & vertex buffer.
																		 // for now we want to keep the buffer spetilised to one type as this will allow vulkan to optimize the data.
		VK_SHARING_MODE_EXCLUSIVE,                                       // There are two modes, exclusive and concurrent. Defines if it can concurrently be used by multiple queue
																		 // families at the same time
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT                              // What properties do we rquire of our memory
	);

	// Get the pointer to the GPU memory
	VkResult mapped_memory_result = vkMapMemory(
		device,                                                         // The device that the memory is on
		buffer_memory,                                                  // The device memory instance
		0,                                                              // Offset from the memorys start that we are accessing
		buffer_size,                                                    // How much memory are we accessing
		0,                                                              // Flags (we dont need this for basic buffers)
		&mapped_buffer_memory                                           // The return for the memory pointer
	);

	// Could we map the GPU memory to our CPU accessable pointer
	assert(mapped_memory_result == VK_SUCCESS);

	// First we copy the example data to the GPU
	memcpy(
		mapped_buffer_memory,                                           // The destination for our memory (GPU)
		example_input_buffer_data,                                      // Source for the memory (CPU-Ram)
		buffer_size                                                     // How much data we are transfering
	);


	// Define how bing the descriptor pool will be
	VkDescriptorPoolSize pool_size[1] = { VkHelper::DescriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1) };

	// Create a descriptor pool of x size
	descriptor_pool = VkHelper::CreateDescriptorPool(device, pool_size, 1, 100);


	VkDescriptorSetLayoutBinding layout_bindings[1] = { VkHelper::DescriptorSetLayoutBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT) };

	descriptor_set_layout = VkHelper::CreateDescriptorSetLayout(device, layout_bindings, 1);

	descriptor_set = VkHelper::AllocateDescriptorSet(device, descriptor_pool, descriptor_set_layout, 1);

	{ // Update the Descriptor Set
		VkDescriptorBufferInfo buffer_info = {};
		buffer_info.buffer = buffer;
		buffer_info.offset = 0;
		buffer_info.range = buffer_size;

		descriptor_write_set = {};
		descriptor_write_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptor_write_set.dstSet = descriptor_set;
		descriptor_write_set.dstBinding = 0;
		descriptor_write_set.dstArrayElement = 0;
		descriptor_write_set.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptor_write_set.descriptorCount = 1;
		descriptor_write_set.pBufferInfo = &buffer_info;

		vkUpdateDescriptorSets(
			device,
			1, // Passing over 1 buffer
			&descriptor_write_set,
			0,
			NULL
		);
	}
}

// Everything within the Destroy is from previous tuturials
// Destroy
// - Buffer
// - Command Pool
// - Device
// - Debugger
// - Instance
// - Descriptor
void Destroy()
{

	vkDestroyDescriptorSetLayout(
		device,
		descriptor_set_layout,
		nullptr
	);
	vkDestroyDescriptorPool(
		device,
		descriptor_pool,
		nullptr
	);

	// Now we unmap the data
	vkUnmapMemory( 
		device,
		buffer_memory
	);

	// Clean up the buffer data
	vkDestroyBuffer(
		device,
		buffer,
		nullptr
	);

	// Free the memory that was allocated for the buffer
	vkFreeMemory(
		device,
		buffer_memory,
		nullptr
	);

	// Clean up the command pool
	vkDestroyCommandPool(
		device,
		command_pool,
		nullptr
	);

	// Clean up the device now that the project is stopping
	vkDestroyDevice(
		device,
		nullptr
	);

	// Destroy the debug callback
	// We cant directly call vkDestroyDebugReportCallbackEXT as we need to find the pointer within the Vulkan DLL, See function inplmentation for details.
	VkHelper::DestroyDebugger(
		instance, 
		debugger
	);

	// Clean up the vulkan instance
	vkDestroyInstance(
		instance,
		NULL
	);
}

bool CheckSwapchainSupport(VkSurfaceCapabilitiesKHR& capabilities, VkSurfaceFormatKHR*& formats, 
	uint32_t& format_count, VkPresentModeKHR*& modes, uint32_t& mode_count)
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

VkSurfaceFormatKHR ChooseSwapchainSurfaceFormat(const VkSurfaceFormatKHR* formats,const uint32_t& format_count)
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

VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR & capabilities)
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

int main(int argc, char **argv)
{
	WindowSetup("Vulkan", 1080, 720);

	// The window surface creation was added in the setup code as it must happen before the physical device is chosen and after the instance is created
	// Setup the components from the previous projects
	Setup();





	VkSurfaceCapabilitiesKHR capabilities;
	VkSurfaceFormatKHR* surface_formats = nullptr;
	uint32_t surface_format_count = 0;
	VkPresentModeKHR* present_modes = nullptr;
	uint32_t present_mode_count = 0;

	bool hasSupport = CheckSwapchainSupport(capabilities, surface_formats, surface_format_count, present_modes, present_mode_count);

	assert(hasSupport);


	surface_format = ChooseSwapchainSurfaceFormat(surface_formats, surface_format_count);
	present_mode = ChooseSwapPresentMode(present_modes, present_mode_count);

	VkExtent2D extent = ChooseSwapExtent(capabilities);

	// Make sure that the amount of images we are creating for the swapchain are in between the min and max
	uint32_t image_count = capabilities.minImageCount + 1;
	if (capabilities.maxImageCount > 0 && image_count > capabilities.maxImageCount)
	{
		image_count = capabilities.maxImageCount;
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

	create_info.preTransform = capabilities.currentTransform; // We don't want any transformations applied to the images
	create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; // Ignore the alpha channel so we can do blending


	// If we support transfering data from a buffer to this image, then we can enable the feature on the swapchain image
	if ((capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT) == VK_IMAGE_USAGE_TRANSFER_DST_BIT)
	{
		create_info.imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	}

	// If we support transfering data from the swapchain image to a buffer, then we can enable the feature on the swapchain image
	if ((capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) == VK_IMAGE_USAGE_TRANSFER_SRC_BIT)
	{
		create_info.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	}

	// Create a new swapchain using the constructor structure
	VkResult create_swapchain_resut =  vkCreateSwapchainKHR(
		device,
		&create_info,
		nullptr,
		&swap_chain
	);

	// Validate the swapchain was created, do this at debug time
	assert(create_swapchain_resut == VK_SUCCESS);

	// Get the swapchain image count
	VkResult get_swapchain_images_sucsess = vkGetSwapchainImagesKHR(
		device,
		swap_chain,
		&swapchain_image_count,
		nullptr
	);

	// Make sure the swapchain image count was returned correctly
	assert(get_swapchain_images_sucsess == VK_SUCCESS);

	swapchain_images = std::unique_ptr<VkImage>(new VkImage[swapchain_image_count]);

	// Get the refrence back to the swapchain images
	get_swapchain_images_sucsess = vkGetSwapchainImagesKHR(
		device,
		swap_chain,
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




	/////////////////////////////////////////
	///// Finished setting up Swapchain ///// 
	/////////////////////////////////////////








	for (uint32_t i = 0; i < swapchain_image_count; i++)
	{
		vkDestroyImageView(
			device,
			swapchain_image_views.get()[i],
			nullptr
		);
	}


	vkDestroySwapchainKHR(
		device,
		swap_chain,
		nullptr
	);



	DestroyWindow();


	// Finish previous projects cleanups
	Destroy();

	return 0;
}