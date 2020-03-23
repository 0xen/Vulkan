
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

void CreateRenderResources();
void DestroyRenderResources();
void RebuildRenderResources();
void BuildCommandBuffers(std::unique_ptr<VkCommandBuffer>& command_buffers, const uint32_t buffer_count);

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


VkPipelineStageFlags wait_stages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
VkPresentInfoKHR present_info = {};
VkSubmitInfo sumbit_info = {};
std::unique_ptr<VkCommandBuffer> graphics_command_buffers = nullptr;


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
				window_width = event.window.data1;
				window_height = event.window.data2;
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


	CreateRenderResources();

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

	DestroyRenderResources();

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



void CreateRenderResources()
{

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

	sumbit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	sumbit_info.waitSemaphoreCount = 1;
	sumbit_info.pWaitDstStageMask = &wait_stages;
	sumbit_info.commandBufferCount = 1;
	sumbit_info.signalSemaphoreCount = 1;


	present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.waitSemaphoreCount = 1;
	present_info.swapchainCount = 1;
	present_info.pSwapchains = &swap_chain;
	present_info.pResults = nullptr;

	// All three semaphores are defined in the main as they are a part of the tuturial on how to create them
	//sumbit_info.pSignalSemaphores = nullptr;
	//sumbit_info.pWaitSemaphores = nullptr;
	//present_info.pWaitSemaphores = nullptr;
}

void DestroyRenderResources()
{

	vkDestroyRenderPass(
		device,
		renderpass,
		nullptr
	);

	vkDestroySwapchainKHR(
		device,
		swap_chain,
		nullptr
	);

	for (int i = 0; i < swapchain_image_count; i++)
	{
		vkDestroyImageView(
			device,
			swapchain_image_views.get()[i],
			nullptr
		);

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

}


void RebuildRenderResources()
{
	VkResult device_idle_result = vkDeviceWaitIdle(device);
	assert(device_idle_result == VK_SUCCESS);

	DestroyRenderResources();
	CreateRenderResources();
	BuildCommandBuffers(graphics_command_buffers, swapchain_image_count);
}



void BuildCommandBuffers(std::unique_ptr<VkCommandBuffer>& command_buffers, const uint32_t buffer_count)
{
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


	for (unsigned int i = 0; i < buffer_count; i++)
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


	graphics_command_buffers = std::unique_ptr<VkCommandBuffer>(new VkCommandBuffer[swapchain_image_count]);
	
	VkCommandBufferAllocateInfo command_buffer_allocate_info = VkHelper::CommandBufferAllocateInfo(
		command_pool,
		swapchain_image_count
	);

	VkResult allocate_command_buffer_resut = vkAllocateCommandBuffers(
		device,
		&command_buffer_allocate_info,
		graphics_command_buffers.get()
	);
	assert(allocate_command_buffer_resut == VK_SUCCESS);
	


	BuildCommandBuffers(graphics_command_buffers, swapchain_image_count);

	// sumbit_info and present_info are defined within CreateRenderResources as they need to get the
	// newly created swapchain instance

	// We need to attach our demaphores to the pre defined structures from before
	sumbit_info.pSignalSemaphores = &render_finished_semaphore;
	sumbit_info.pWaitSemaphores = &image_available_semaphore;

	present_info.pWaitSemaphores = &render_finished_semaphore;

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



		sumbit_info.pCommandBuffers = &graphics_command_buffers.get()[current_frame_index];

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
		// If the window was resized or something else made the current render invalid, we need to rebuild all the
		// render resources
		if (queue_present_result == VK_ERROR_OUT_OF_DATE_KHR)
		{
			RebuildRenderResources();
		}

		assert(queue_present_result == VK_SUCCESS || queue_present_result == VK_ERROR_OUT_OF_DATE_KHR);

		VkResult device_idle_result = vkDeviceWaitIdle(device);
		assert(device_idle_result == VK_SUCCESS);
	}



	vkDestroySemaphore(
		device,
		image_available_semaphore,
		nullptr
	);

	vkDestroySemaphore(
		device,
		render_finished_semaphore,
		nullptr
	);

	for (unsigned int i = 0; i < swapchain_image_count; i++)
	{
		vkDestroyFence(
			device,
			fences.get()[i],
			nullptr
		);
	}



	// Finish previous projects cleanups
	Destroy();

	return 0;
}