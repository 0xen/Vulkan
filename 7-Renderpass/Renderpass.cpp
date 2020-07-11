
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
VkQueue graphics_queue = VK_NULL_HANDLE;
VkQueue present_queue = VK_NULL_HANDLE;
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
VkSurfaceCapabilitiesKHR surface_capabilities;
VkSurfaceKHR surface;
uint32_t window_width;
uint32_t window_height;

VkSurfaceFormatKHR surface_format;
VkPresentModeKHR present_mode;

VkSwapchainKHR swap_chain;
std::unique_ptr<VkImage> swapchain_images;
uint32_t swapchain_image_count;
std::unique_ptr<VkImageView> swapchain_image_views;


struct VulkanAttachments
{
	FrameBufferAttachment color, depth;
};

// What format will the screen space be in
const VkFormat colorFormat = VK_FORMAT_R8G8B8A8_UNORM;

VkRenderPass renderpass = VK_NULL_HANDLE;
std::unique_ptr<VkFramebuffer> framebuffers = nullptr;
std::unique_ptr<VulkanAttachments> framebuffer_attachments = nullptr;



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
// - Descriptor
// - Swapchain
void Setup()
{

	WindowSetup("Vulkan", 1080, 720);


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
		"7 Renderpass", VK_MAKE_VERSION(1, 0, 0),
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

	vkGetDeviceQueue(
		device,
		physical_devices_queue_family,
		0,
		&graphics_queue
	);

	vkGetDeviceQueue(
		device,
		physical_devices_queue_family,
		0,
		&present_queue
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




	VkDescriptorPoolSize pool_size[1] = { VkHelper::DescriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1) };

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



	swap_chain = VkHelper::CreateSwapchain(
		physical_device,
		device,
		surface,
		surface_capabilities,
		surface_format,
		present_mode,
		window_width,
		window_height,
		swapchain_image_count,
		swapchain_images,
		swapchain_image_views
	);


}

// Everything within the Destroy is from previous tuturials
// Destroy
// - Swapchain
// - Descriptor
// - Buffer
// - Command Pool
// - Device
// - Debugger
// - Instance
void Destroy()
{

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

	DestroyWindow();
}

VkFormat FindSupportedFormat(VkPhysicalDevice physical_device, const VkFormat* candidate_formats, const uint32_t candidate_format_count, VkImageTiling tiling, VkFormatFeatureFlags features)
{
	// Loop through the formats we are considering
	for(int i = 0 ; i < candidate_format_count; i++)
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

struct FrameBufferAttachment
{
	VkImage image;
	VkDeviceMemory memory;
	VkImageView view;
	VkFormat format;
	VkSampler sampler;
};

VkCommandBuffer BeginSingleTimeCommands(VkCommandPool command_pool)
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

void EndSingleTimeCommands(VkCommandBuffer command_buffer, VkCommandPool command_pool)
{
	// End the command buffer
	vkEndCommandBuffer(command_buffer);
	// Submit the command buffer to the queue
	VkSubmitInfo submit_info = VkHelper::SubmitInfo(command_buffer);
	VkResult queue_submit_result = vkQueueSubmit(
		graphics_queue,
		1,
		&submit_info,
		VK_NULL_HANDLE
	);
	// Validate it submitted alright
	assert(queue_submit_result == VK_SUCCESS);
	// Wait for the GPU
	vkQueueWaitIdle(
		graphics_queue
	);
	// Release the command buffers
	vkFreeCommandBuffers(
		device,
		command_pool,
		1,
		&command_buffer
	);
}


void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout old_layout, VkImageLayout new_layout, VkImageSubresourceRange subresourceRange)
{
	// Create a new single time command
	VkCommandBuffer command_buffer = BeginSingleTimeCommands(command_pool);

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
	EndSingleTimeCommands(command_buffer, command_pool);
}

void CreateAttachmentImages(VkFormat format, VkImageUsageFlags usage, FrameBufferAttachment & attachment)
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
		window_width,
		window_height,
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
		VkResult create_sampler_result =  vkCreateSampler(
			device,
			&sampler_info,
			nullptr,
			&attachment.sampler
		);
		assert(create_sampler_result == VK_SUCCESS);
	}
	// If the image is used for color. switch the images layout
	if (usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
		TransitionImageLayout(attachment.image, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });
}




int main(int argc, char **argv)
{

	Setup();


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
		present_attachment.format = surface_format.format;
		present_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
		present_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;				// Each subpass (or frame as we only have 1 subpass) we want to clear the last present
		present_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;				// What do we want to do with the data after the subpass, we want to store it away
		present_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;		// Dont use stencil currently
		present_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;	// Dont use stencil currently 
		present_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;			// The layout the attachment image subresource, in out case its undefined
		present_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;		// Upon finishing what format do we want, since we are presenting, we need PRESENNT_KHR
	}

	{
		color_attachment.format = colorFormat;
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
		CreateAttachmentImages(colorFormat,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, framebuffer_attachments.get()[i].color);

		CreateAttachmentImages(depth_image_format,
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
		framebuffer_info.width = window_width;
		framebuffer_info.height = window_height;
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





	//////////////////////////////////////////
	///// Finished setting up Renderpass ///// 
	//////////////////////////////////////////





	for (int i = 0; i < swapchain_image_count; i++)
	{
		vkDestroyFramebuffer(
			device,
			framebuffers.get()[i],
			nullptr
		);


		vkDestroyImageView(
			device,
			framebuffer_attachments.get()[i].color.view,
			nullptr
		);
		vkDestroyImage(
			device,
			framebuffer_attachments.get()[i].color.image,
			nullptr
		);
		vkFreeMemory(
			device,
			framebuffer_attachments.get()[i].color.memory,
			nullptr
		);
		vkDestroySampler(
			device,
			framebuffer_attachments.get()[i].color.sampler,
			nullptr
		);
		vkDestroyImageView(
			device,
			framebuffer_attachments.get()[i].depth.view,
			nullptr
		);
		vkDestroyImage(
			device,
			framebuffer_attachments.get()[i].depth.image,
			nullptr
		);
		vkFreeMemory(
			device,
			framebuffer_attachments.get()[i].depth.memory,
			nullptr
		);
		vkDestroySampler(
			device,
			framebuffer_attachments.get()[i].depth.sampler,
			nullptr
		);
	}


	vkDestroyRenderPass(
		device,
		renderpass,
		nullptr
	);


	// Finish previous projects cleanups
	Destroy();

	return 0;
}