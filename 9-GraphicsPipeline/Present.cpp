
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

bool window_open;
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

VkRenderPass renderpass = VK_NULL_HANDLE;
std::unique_ptr<VkFramebuffer> framebuffers = nullptr;
std::unique_ptr<VkHelper::VulkanAttachments> framebuffer_attachments = nullptr;


void WindowSetup(const char* title, int width, int height)
{
	window = SDL_CreateWindow(
		title,
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		width, height,
		SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE
	);
	SDL_ShowWindow(window);
	window_open = true;

	window_width = width;
	window_height = height;

	SDL_VERSION(&window_info.version);
	bool sucsess = SDL_GetWindowWMInfo(window, &window_info);
	assert(sucsess && "Error, unable to get window info");
}

void PollWindow()
{

	// Poll Window
	SDL_Event event;
	bool rebuild = false;
	while (SDL_PollEvent(&event) > 0)
	{
		switch (event.type)
		{
		case SDL_QUIT:
			window_open = false;
			break;
		case SDL_WINDOWEVENT:
			switch (event.window.event)
			{
				//Get new dimensions and repaint on window size change
			case SDL_WINDOWEVENT_SIZE_CHANGED:

				Sint32 width = event.window.data1;
				Sint32 height = event.window.data2;

				break;
			}
			break;
		}
	}
}
void DestroyWindow()
{
	SDL_DestroyWindow(window);
}

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
		"2 - Device", VK_MAKE_VERSION(1, 0, 0),
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


	const VkFormat colorFormat = VK_FORMAT_R8G8B8A8_UNORM;
	renderpass = VkHelper::CreateRenderPass(
		physical_device,
		device,
		surface_format.format,
		colorFormat,
		swapchain_image_count,
		physical_device_mem_properties,
		physical_device_features,
		physical_device_properties,
		command_pool,
		graphics_queue,
		window_width,
		window_height,
		framebuffers,
		framebuffer_attachments,
		swapchain_image_views
	);

}

// Everything within the Destroy is from previous tuturials
// Destroy
// - Buffer
// - Command Pool
// - Device
// - Debugger
// - Instance
void Destroy()
{
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




int main(int argc, char **argv)
{

	Setup();

	uint32_t current_frame_index = 0; // What frame are we currently using

	std::unique_ptr<VkFence> fences = std::unique_ptr<VkFence>(new VkFence[swapchain_image_count]);

	for (unsigned int i = 0; i < swapchain_image_count; i++)
	{
		VkFenceCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
		VkResult create_fence_result = vkCreateFence(
			device,
			&info,
			nullptr,
			&fences.get()[i]
		);
		assert(create_fence_result == VK_SUCCESS);
	}

	VkSemaphore image_available_semaphore;
	VkSemaphore render_finished_semaphore;

	VkSemaphoreCreateInfo semaphore_create_info = {};
	semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkResult create_semaphore_result = vkCreateSemaphore(device,
		&semaphore_create_info,
		nullptr,
		&image_available_semaphore
	);
	assert(create_semaphore_result == VK_SUCCESS);

	create_semaphore_result = vkCreateSemaphore(device,
		&semaphore_create_info,
		nullptr,
		&render_finished_semaphore
	);
	assert(create_semaphore_result == VK_SUCCESS);





	std::unique_ptr<VkCommandBuffer> command_buffers = std::unique_ptr<VkCommandBuffer>(new VkCommandBuffer[swapchain_image_count]);
	
	VkCommandBufferAllocateInfo command_buffer_allocate_info = VkHelper::CommandBufferAllocateInfo(
		command_pool,
		swapchain_image_count
	);

	VkResult allocate_command_buffer_resut = vkAllocateCommandBuffers(
		device,
		&command_buffer_allocate_info,
		command_buffers.get()
	);
	assert(allocate_command_buffer_resut == VK_SUCCESS);
	


	VkCommandBufferBeginInfo command_buffer_begin_info = {};
	command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
	command_buffer_begin_info.pInheritanceInfo = nullptr;


	float clear_color[4] = { 1.0f,1.0f,0.0f,1.0f };

	VkClearValue clear_values[3]{};

	std::copy(std::begin(clear_color), std::end(clear_color), std::begin(clear_values[0].color.float32)); // Present
	std::copy(std::begin(clear_color), std::end(clear_color), std::begin(clear_values[1].color.float32)); // Color Image
	clear_values[2].depthStencil = { 1.0f, 0 };                                                           // Depth Image


	VkRenderPassBeginInfo render_pass_info = {};
	render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	render_pass_info.renderPass = renderpass;
	render_pass_info.renderArea.offset = { 0, 0 };
	render_pass_info.renderArea.extent = { window_width, window_height };
	render_pass_info.clearValueCount = 3;
	render_pass_info.pClearValues = clear_values;

	VkImageSubresourceRange subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };


	for (unsigned int i = 0; i < swapchain_image_count; i++)
	{
		// Reset the command buffers
		vkResetCommandBuffer(
			command_buffers.get()[i],
			0
		);

		render_pass_info.framebuffer = framebuffers.get()[i];

		VkResult begin_command_buffer_result = vkBeginCommandBuffer(
			command_buffers.get()[i],
			&command_buffer_begin_info
		);
		assert(begin_command_buffer_result == VK_SUCCESS);


		vkCmdBeginRenderPass(
			command_buffers.get()[i],
			&render_pass_info,
			VK_SUBPASS_CONTENTS_INLINE
		);


		vkCmdSetLineWidth(
			command_buffers.get()[i],
			1.0f
		);
		
		VkViewport viewport = {};
		viewport.x = 0;
		viewport.y = 0;
		viewport.width = window_width;
		viewport.height = window_height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		vkCmdSetViewport(
			command_buffers.get()[i],
			0,
			1,
			&viewport
		);
		VkRect2D scissor{};
		scissor.extent.width = window_width;
		scissor.extent.height = window_height;
		scissor.offset.x = 0;
		scissor.offset.y = 0;

		vkCmdSetScissor(
			command_buffers.get()[i],
			0,
			1,
			&scissor
		);






		// To do


		vkCmdEndRenderPass(
			command_buffers.get()[i]
		);

		VkResult end_command_buffer_result = vkEndCommandBuffer(
			command_buffers.get()[i]
		);
		assert(end_command_buffer_result == VK_SUCCESS);

	}


	VkPipelineStageFlags wait_stages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	VkSubmitInfo sumbit_info = {};
	sumbit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	sumbit_info.waitSemaphoreCount = 1;
	sumbit_info.pWaitSemaphores = &image_available_semaphore;
	sumbit_info.pWaitDstStageMask = &wait_stages;
	sumbit_info.commandBufferCount = 1;
	sumbit_info.signalSemaphoreCount = 1;
	sumbit_info.pSignalSemaphores = &render_finished_semaphore;


	VkSwapchainKHR swap_chains[] = { swap_chain };
	VkPresentInfoKHR present_info = {};
	present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.waitSemaphoreCount = 1;
	present_info.pWaitSemaphores = &render_finished_semaphore;
	present_info.swapchainCount = 1;
	present_info.pSwapchains = swap_chains;
	present_info.pResults = nullptr;

	while (window_open)
	{
		PollWindow();


		// Find next image


		vkWaitForFences(
			device,
			1,
			&fences.get()[current_frame_index],
			VK_TRUE,
			UINT32_MAX
		);
		VkResult acquire_next_image_result = vkAcquireNextImageKHR(
			device,
			swap_chain,
			UINT64_MAX,
			image_available_semaphore,
			VK_NULL_HANDLE,
			&current_frame_index
		);
		assert(acquire_next_image_result == VK_SUCCESS);

		// Reset and wait
		VkResult queue_idle_result = vkQueueWaitIdle(
			graphics_queue
		);
		assert(queue_idle_result == VK_SUCCESS);

		VkResult reset_fences_result = vkResetFences(
			device,
			1,
			&fences.get()[current_frame_index]
		);
		assert(reset_fences_result == VK_SUCCESS);



		sumbit_info.pCommandBuffers = &command_buffers.get()[current_frame_index];

		VkResult queue_submit_result = vkQueueSubmit(
			graphics_queue,
			1,
			&sumbit_info,
			fences.get()[current_frame_index]
		);
		assert(queue_submit_result == VK_SUCCESS);



		present_info.pImageIndices = &current_frame_index;


		VkResult queue_present_result = vkQueuePresentKHR(
			present_queue,
			&present_info
		);
		assert(queue_present_result == VK_SUCCESS);

		VkResult device_idle_result = vkDeviceWaitIdle(device);
		assert(device_idle_result == VK_SUCCESS);
	}







	// Finish previous projects cleanups
	Destroy();

	return 0;
}