
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


struct VertexData
{
	float position[3];
	float color[3];
	float padding[2];
};

VertexData verticies[3] =
{
	{{0.0f,-0.5f,0.0f},{1.0f,0.0f,0.0f}},
	{{0.5f,0.5f,0.0f},{0.0f,1.0f,0.0f}},
	{{-0.5f,0.5f,0.0f},{0.0f,0.0f,1.0f}}
};

uint32_t indicies[3] = {
	0,1,2
};

VkDeviceSize vertex_buffer_size = sizeof(VertexData) * 3;
VkDeviceSize index_buffer_size = sizeof(uint32_t) * 3;

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

uint32_t current_frame_index = 0; // What frame are we currently using
std::unique_ptr<VkFence> fences = nullptr;

VkSemaphore image_available_semaphore;
VkSemaphore render_finished_semaphore;
VkSubmitInfo render_submission_info = {};
VkPresentInfoKHR present_info = {};
VkPipelineStageFlags wait_stages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

std::unique_ptr<VkCommandBuffer> graphics_command_buffers = nullptr;


VkPipeline graphics_pipeline = VK_NULL_HANDLE;
VkPipelineLayout graphics_pipeline_layout = VK_NULL_HANDLE;
VkShaderModule vertex_shader_module = VK_NULL_HANDLE;
VkShaderModule fragment_shader_module = VK_NULL_HANDLE;


VkBuffer vertex_buffer = VK_NULL_HANDLE;
VkDeviceMemory vertex_buffer_memory = VK_NULL_HANDLE;
// Raw pointer that will point to GPU memory
void* vertex_mapped_buffer_memory = nullptr;


VkBuffer index_buffer = VK_NULL_HANDLE;
VkDeviceMemory index_buffer_memory = VK_NULL_HANDLE;
// Raw pointer that will point to GPU memory
void* index_mapped_buffer_memory = nullptr;


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

	window_open = true;
}

// Check for window updates and process them
void PollWindow()
{

	// Poll Window
	SDL_Event event;
	bool rebuild = false;
	while (SDL_PollEvent(&event) > 0)
	{
		switch (event.type)
		{
		case SDL_QUIT: // On quit, define that the window is closed
			window_open = false;
			break;
		case SDL_WINDOWEVENT:
			switch (event.window.event)
			{
				//Get new dimensions and repaint on window size change
			case SDL_WINDOWEVENT_SIZE_CHANGED:

				window_width = event.window.data1;
				window_height = event.window.data2;
				RebuildRenderResources();
				break;
			}
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
// - Renderpass
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
		"9 - Graphics Pipeline", VK_MAKE_VERSION(1, 0, 0),
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



	// Create the fences required for the swapchain rendering 
	VkHelper::CreateFence(device, fences, swapchain_image_count);

	// Create the image avaliable and finished semaphore
	VkHelper::CreateVkSemaphore(device, image_available_semaphore);
	VkHelper::CreateVkSemaphore(device, render_finished_semaphore);

	VkCommandBufferAllocateInfo command_buffer_allocate_info = VkHelper::CommandBufferAllocateInfo(
		command_pool,
		swapchain_image_count
	);

	graphics_command_buffers = std::unique_ptr<VkCommandBuffer>(new VkCommandBuffer[swapchain_image_count]);

	VkResult allocate_command_buffer_resut = vkAllocateCommandBuffers(
		device,
		&command_buffer_allocate_info,
		graphics_command_buffers.get()
	);
	assert(allocate_command_buffer_resut == VK_SUCCESS);

	render_submission_info = VkHelper::SubmitInfo(1, &image_available_semaphore, 1, &render_finished_semaphore, wait_stages);

	present_info = VkHelper::PresentInfoKHR(1, &render_finished_semaphore, swap_chain);

}

// Everything within the Destroy is from previous tuturials
// Destroy
// - Renderpass
// - Swapchain
// - Descriptor
// - Buffer
// - Command Pool
// - Device
// - Debugger
// - Instance
void Destroy()
{
	DestroyRenderResources();

	for (uint32_t i = 0; i < swapchain_image_count; i++)
	{
		vkDestroyFence(
			device,
			fences.get()[i],
			nullptr
		);
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

	for (uint32_t i = 0; i < swapchain_image_count; i++)
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
	// Wait for the device to finish
	VkResult device_idle_result = vkDeviceWaitIdle(device);
	assert(device_idle_result == VK_SUCCESS);
	// Destroy and rebuild the render resources
	DestroyRenderResources();
	CreateRenderResources();
	BuildCommandBuffers(graphics_command_buffers, swapchain_image_count);
}

void BuildCommandBuffers(std::unique_ptr<VkCommandBuffer>& command_buffers, const uint32_t buffer_count)
{
	// Define how we will start the command buffer recording
	VkCommandBufferBeginInfo command_buffer_begin_info = VkHelper::CommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT);

	// Define the clear color of the screen
	const float clear_color[4] = { 0.2f,0.2f,0.2f,1.0f };

	VkClearValue clear_values[3]{};

	// Copy over the clear color values to the clear color structure
	std::copy(std::begin(clear_color), std::end(clear_color), std::begin(clear_values[0].color.float32)); // Present
	std::copy(std::begin(clear_color), std::end(clear_color), std::begin(clear_values[1].color.float32)); // Color Image
	clear_values[2].depthStencil = { 1.0f, 0 };                                                           // Depth Image


	// Define the render pass begin code
	VkRenderPassBeginInfo render_pass_info = VkHelper::RenderPassBeginInfo(
		renderpass,
		{ 0,0 },
		{ window_width, window_height }
	);

	render_pass_info.clearValueCount = 3;
	render_pass_info.pClearValues = clear_values;

	VkImageSubresourceRange subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

	// Define the size of the viewport we are rendering too
	VkViewport viewport = VkHelper::Viewport(
		window_width,
		window_height,
		0,
		0,
		0.0f,
		1.0f
	);

	// Define what area of the screen should be kept and what area should be cut
	VkRect2D scissor = VkHelper::Scissor(
		window_width,
		window_height,
		0,
		0
	);

	VkDeviceSize offsets[] = { 0 };

	// Loop through for the swapchain buffers
	for (unsigned int i = 0; i < buffer_count; i++)
	{
		// Reset the command buffers
		VkResult reset_command_buffer_result = vkResetCommandBuffer(
			command_buffers.get()[i],
			0
		);
		assert(reset_command_buffer_result == VK_SUCCESS);

		// Define the frame buffer we want to use
		render_pass_info.framebuffer = framebuffers.get()[i];

		// Begin the new command buffer
		VkResult begin_command_buffer_result = vkBeginCommandBuffer(
			command_buffers.get()[i],
			&command_buffer_begin_info
		);
		// check to see if the command buffer was created
		assert(begin_command_buffer_result == VK_SUCCESS);

		// Define that we will be starting a new render pass
		vkCmdBeginRenderPass(
			command_buffers.get()[i],
			&render_pass_info,
			VK_SUBPASS_CONTENTS_INLINE
		);

		// Set the viewport
		vkCmdSetViewport(
			command_buffers.get()[i],
			0,
			1,
			&viewport
		);

		// Set the scissor region
		vkCmdSetScissor(
			command_buffers.get()[i],
			0,
			1,
			&scissor
		);

		// Bind the graphics pipeline
		vkCmdBindPipeline(
			command_buffers.get()[i],
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			graphics_pipeline
		);

		// Bind the models vertex buffer
		vkCmdBindVertexBuffers(
			command_buffers.get()[i],
			0,
			1,
			&vertex_buffer,
			offsets
		);

		// Bind the models index buffer
		vkCmdBindIndexBuffer(
			command_buffers.get()[i],
			index_buffer,
			0,
			VK_INDEX_TYPE_UINT32
		);

		// Draw the model
		vkCmdDrawIndexed(
			command_buffers.get()[i],
			3,							// The model consists of 3 verticies
			1,							// Draw once model
			0,
			0,
			0
		);


		// End the rendering
		vkCmdEndRenderPass(
			command_buffers.get()[i]
		);

		// End the current rendering command
		VkResult end_command_buffer_result = vkEndCommandBuffer(
			command_buffers.get()[i]
		);
		assert(end_command_buffer_result == VK_SUCCESS);
	}
}

void Render()
{
	// Find next image
	VkResult wait_for_fences = vkWaitForFences(
		device,
		1,
		&fences.get()[current_frame_index],
		VK_TRUE,
		UINT32_MAX
	);
	assert(wait_for_fences == VK_SUCCESS);

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

	render_submission_info.pCommandBuffers = &graphics_command_buffers.get()[current_frame_index];

	VkResult queue_submit_result = vkQueueSubmit(
		graphics_queue,
		1,
		&render_submission_info,
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

int main(int argc, char **argv)
{

	Setup();

	// Create a peipeline layout, a pipeline layout defines what descriptors to accept
	VkPipelineLayoutCreateInfo pipeline_layout_info = {};
	pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipeline_layout_info.setLayoutCount = 0;									// How many descriptor set layouts are we expecting
	pipeline_layout_info.pSetLayouts = nullptr;									// Pointer to the descriptor set layouts
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


	// How many shaders will the pipeline use
	unsigned int shader_stage_count = 2;

	// Create pointer array of shader stage create infos
	std::unique_ptr<VkPipelineShaderStageCreateInfo> shader_stages =
		std::unique_ptr<VkPipelineShaderStageCreateInfo>(new VkPipelineShaderStageCreateInfo[shader_stage_count]);


	char* vertex_shader_data = nullptr;
	unsigned int vertex_shader_size = 0;
	// Load the vertex shader
	VkHelper::ReadShaderFile(
		"../../Data/Shaders/9-GraphicsPipeline/vert.spv",
		vertex_shader_data,
		vertex_shader_size
	);

	char* fragment_shader_data = nullptr;
	unsigned int fragment_shader_size = 0;
	// Load the fragment shader
	VkHelper::ReadShaderFile(
		"../../Data/Shaders/9-GraphicsPipeline/frag.spv",
		fragment_shader_data,
		fragment_shader_size
	);

	// Create the vertex shader module
	VkShaderModuleCreateInfo vertex_shader_module_create_info = {};
	vertex_shader_module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	vertex_shader_module_create_info.codeSize = vertex_shader_size;
	vertex_shader_module_create_info.pCode = reinterpret_cast<const uint32_t*>(vertex_shader_data);

	// Create the fragment shader module
	VkShaderModuleCreateInfo fragment_shader_module_create_info = {};
	fragment_shader_module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	fragment_shader_module_create_info.codeSize = fragment_shader_size;
	fragment_shader_module_create_info.pCode = reinterpret_cast<const uint32_t*>(fragment_shader_data);

	// Create the shader
	VkResult create_shader_module = vkCreateShaderModule(
		device,
		&vertex_shader_module_create_info,
		nullptr,
		&vertex_shader_module
	);
	// Validate the shader module
	assert(create_shader_module == VK_SUCCESS);

	// Create the shader
	create_shader_module = vkCreateShaderModule(
		device,
		&fragment_shader_module_create_info,
		nullptr,
		&fragment_shader_module
	);
	// Validate the shader module
	assert(create_shader_module == VK_SUCCESS);

	// Define the vertex shader create info
	shader_stages.get()[0] = {};
	shader_stages.get()[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shader_stages.get()[0].pName = "main";												// In the shader whats the entry function name
	shader_stages.get()[0].stage = VK_SHADER_STAGE_VERTEX_BIT;							// Define what type of shader we are making
	shader_stages.get()[0].module = vertex_shader_module;

	// Define the fragment shader create info
	shader_stages.get()[1] = {};
	shader_stages.get()[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shader_stages.get()[1].pName = "main";													// In the shader whats the entry function name
	shader_stages.get()[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;							// Define what type of shader we are making
	shader_stages.get()[1].module = fragment_shader_module;

	// How many sets of vertex data will be coming into the pipeline at once
	const uint32_t vertex_input_binding_description_count = 1;
	// Create a pointer list of input bindings
	std::unique_ptr<VkVertexInputBindingDescription> vertex_input_binding_descriptions =
		std::unique_ptr<VkVertexInputBindingDescription>(new VkVertexInputBindingDescription[vertex_input_binding_description_count]);

	vertex_input_binding_descriptions.get()[0].binding = 0;								// Define the binding
	vertex_input_binding_descriptions.get()[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;	// How oftern will the data come into the pipeline, since this is 
																						// the models "vertex data", we want it to come in one vertex at a time
	vertex_input_binding_descriptions.get()[0].stride = sizeof(VertexData);				// How big is the vertex data packet


	// Define hoe many input attribute descriptions we have
	// These descriptions are diffrence to the binding descriptions as before we defined how oftern a new set of vertex data will be used
	// Here we define what the vertex data packet is made up of, for example, Vectex Position, Texture UV, Color, etc
	const uint32_t vertex_input_attribute_description_count = 2;

	// Create a new pointer array of the input attribute description data
	std::unique_ptr<VkVertexInputAttributeDescription> vertex_input_attribute_descriptions =
		std::unique_ptr<VkVertexInputAttributeDescription>(new VkVertexInputAttributeDescription[vertex_input_attribute_description_count]);

	// Used to store the position data of the vertex
	vertex_input_attribute_descriptions.get()[0].binding = 0;								// What vertex input binding are we talking about
	vertex_input_attribute_descriptions.get()[0].location = 0;								// Inside the binding what is its location id
	vertex_input_attribute_descriptions.get()[0].format = VK_FORMAT_R32G32B32_SFLOAT;		// What format is the data coming in as
	vertex_input_attribute_descriptions.get()[0].offset = offsetof(VertexData,position);	// Withn the whole structure of the data packet, where dose it start in memory

	vertex_input_attribute_descriptions.get()[1].binding = 0;								// What vertex input binding are we talking about
	vertex_input_attribute_descriptions.get()[1].location = 1;								// Inside the binding what is its location id
	vertex_input_attribute_descriptions.get()[1].format = VK_FORMAT_R32G32B32_SFLOAT;		// What format is the data coming in as
	vertex_input_attribute_descriptions.get()[1].offset = offsetof(VertexData, color);		// Withn the whole structure of the data packet, where dose it start in memory


	// Create the vertex input state create info to store all data relating to the vertex input
	VkPipelineVertexInputStateCreateInfo vertex_input_info = {};
	vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertex_input_info.vertexBindingDescriptionCount = vertex_input_binding_description_count;
	vertex_input_info.vertexAttributeDescriptionCount = vertex_input_attribute_description_count;
	vertex_input_info.pVertexBindingDescriptions = vertex_input_binding_descriptions.get();
	vertex_input_info.pVertexAttributeDescriptions = vertex_input_attribute_descriptions.get();
	

	// Define what type of data we will be processing and how it will be stitched together. in this instance, its a list of triangles
	VkPipelineInputAssemblyStateCreateInfo input_assembly = {};
	input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
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
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;										// Will the rasterizer break up the triangles into pixels
	rasterizer.lineWidth = 1.0f;														// If lines are drawn (Not VK_POLYGON_MODE_FILL), how wide will they be
	rasterizer.cullMode = VK_CULL_MODE_NONE;											// Will any culling be preformed on the model
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;								// What order is the vertex data coming in, this is to help determan culling
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
	depth_stencil.depthTestEnable = VK_TRUE;											// We want to test for depth
	depth_stencil.depthWriteEnable = VK_TRUE;											// We want to wright depth information to the buffer
	depth_stencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;							// New data should be less then the previous data
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


	// How many dynamic states do we want.
	// Without dynamic states, each time we want to change the draw line width, viewport, scissor, etc, we would have to rebuild the pipeline
	// Here we define that insted of using the values defined here, we take the values recorded in the render pass command buffer recording
	const uint32_t dynamic_state_count = 3;
	// Define the various states we want to be dynamic
	VkDynamicState dynamic_states[dynamic_state_count] = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR,
		VK_DYNAMIC_STATE_LINE_WIDTH
	};

	// Define the dynamic states
	VkPipelineDynamicStateCreateInfo dynamic_state = {};
	dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamic_state.pDynamicStates = dynamic_states;								// Pointer to the states
	dynamic_state.dynamicStateCount = dynamic_state_count;						// How many states do we have
	dynamic_state.flags = 0;


	// Create the entire pipeline create info structure
	VkGraphicsPipelineCreateInfo pipeline_info = {};
	pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipeline_info.stageCount = shader_stage_count;
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
	pipeline_info.subpass = 0; // Deifne what subpass this is, for now we only have one, but when post processing is added, there will be more
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


	// Create the models vertex buffer
	VkHelper::CreateBuffer(
		device,                                                          // What device are we going to use to create the buffer
		physical_device_mem_properties,                                  // What memory properties are avaliable on the device
		vertex_buffer,                                                   // What buffer are we going to be creating
		vertex_buffer_memory,                                            // The output for the buffer memory
		vertex_buffer_size,                                              // How much memory we wish to allocate on the GPU
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,                            // What type of buffer do we want. Buffers can have multiple types, for example, uniform & vertex buffer.
																		 // for now we want to keep the buffer spetilised to one type as this will allow vulkan to optimize the data.
		VK_SHARING_MODE_EXCLUSIVE,                                       // There are two modes, exclusive and concurrent. Defines if it can concurrently be used by multiple queue
																		 // families at the same time
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT                              // What properties do we rquire of our memory
	);

	// Get the pointer to the GPU memory
	VkResult vertex_mapped_memory_result = vkMapMemory(
		device,                                                         // The device that the memory is on
		vertex_buffer_memory,                                           // The device memory instance
		0,                                                              // Offset from the memorys start that we are accessing
		vertex_buffer_size,                                             // How much memory are we accessing
		0,                                                              // Flags (we dont need this for basic buffers)
		&vertex_mapped_buffer_memory                                    // The return for the memory pointer
	);

	// Could we map the GPU memory to our CPU accessable pointer
	assert(vertex_mapped_memory_result == VK_SUCCESS);

	// First we copy the example data to the GPU
	memcpy(
		vertex_mapped_buffer_memory,                                    // The destination for our memory (GPU)
		verticies,                                                      // Source for the memory (CPU-Ram)
		vertex_buffer_size                                              // How much data we are transfering
	);



	// Create the models Index buffer
	VkHelper::CreateBuffer(
		device,                                                          // What device are we going to use to create the buffer
		physical_device_mem_properties,                                  // What memory properties are avaliable on the device
		index_buffer,                                                    // What buffer are we going to be creating
		index_buffer_memory,                                             // The output for the buffer memory
		index_buffer_size,                                               // How much memory we wish to allocate on the GPU
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,                            // What type of buffer do we want. Buffers can have multiple types, for example, uniform & vertex buffer.
																		 // for now we want to keep the buffer spetilised to one type as this will allow vulkan to optimize the data.
		VK_SHARING_MODE_EXCLUSIVE,                                       // There are two modes, exclusive and concurrent. Defines if it can concurrently be used by multiple queue
																		 // families at the same time
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT                              // What properties do we rquire of our memory
	);

	// Get the pointer to the GPU memory
	VkResult index_mapped_memory_result = vkMapMemory(
		device,                                                         // The device that the memory is on
		index_buffer_memory,                                            // The device memory instance
		0,                                                              // Offset from the memorys start that we are accessing
		index_buffer_size,                                              // How much memory are we accessing
		0,                                                              // Flags (we dont need this for basic buffers)
		&index_mapped_buffer_memory                                     // The return for the memory pointer
	);

	// Could we map the GPU memory to our CPU accessable pointer
	assert(index_mapped_memory_result == VK_SUCCESS);

	// First we copy the example data to the GPU
	memcpy(
		index_mapped_buffer_memory,                                           // The destination for our memory (GPU)
		indicies,                                                             // Source for the memory (CPU-Ram)
		index_buffer_size                                                     // How much data we are transfering
	);



	BuildCommandBuffers(graphics_command_buffers, swapchain_image_count);





	/////////////////////////////////////////////////
	///// Finished setting up Graphics Pipeline ///// 
	/////////////////////////////////////////////////




	while (window_open)
	{
		PollWindow();

		Render();
	}



	vkDestroyPipeline(
		device,
		graphics_pipeline,
		nullptr
	);

	vkDestroyPipelineLayout(
		device,
		graphics_pipeline_layout,
		nullptr
	);

	vkDestroyShaderModule(
		device,
		vertex_shader_module,
		nullptr
	);

	vkDestroyShaderModule(
		device,
		fragment_shader_module,
		nullptr
	);


	// Now we unmap the data
	vkUnmapMemory(
		device,
		vertex_buffer_memory
	);

	// Clean up the buffer data
	vkDestroyBuffer(
		device,
		vertex_buffer,
		nullptr
	);

	// Free the memory that was allocated for the buffer
	vkFreeMemory(
		device,
		vertex_buffer_memory,
		nullptr
	);


	// Now we unmap the data
	vkUnmapMemory(
		device,
		index_buffer_memory
	);

	// Clean up the buffer data
	vkDestroyBuffer(
		device,
		index_buffer,
		nullptr
	);

	// Free the memory that was allocated for the buffer
	vkFreeMemory(
		device,
		index_buffer_memory,
		nullptr
	);

	// Finish previous projects cleanups
	Destroy();

	return 0;
}