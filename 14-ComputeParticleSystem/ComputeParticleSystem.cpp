
#define VK_USE_PLATFORM_WIN32_KHR
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <memory>
#include <stdexcept>
#include <iostream>
#include <chrono>
typedef std::chrono::high_resolution_clock Clock;


#define GLM_ENABLE_EXPERIMENTAL
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#include <SDL.h>
#include <SDL_syswm.h>
#include <lodepng.h>
#include <vulkan/vulkan.h>
//#define TINYOBJLOADER_IMPLEMENTATION // define this in only *one* .cc
#include <obj_loader.h>

#include <VkInitializers.hpp>
#include <VkCore.hpp>


#define PARTICLE_COUNT 1000			// How many particles will there be
#define PARTICLE_MAX_LIFE 5.0f		// How long will each particle live for

struct SBuffer
{
	VkBuffer buffer = VK_NULL_HANDLE;
	VkDeviceMemory buffer_memory = VK_NULL_HANDLE;
	unsigned int buffer_size;
	VkBufferUsageFlags usage;
	VkSharingMode sharing_mode;
	VkMemoryPropertyFlags buffer_memory_properties;
	// Raw pointer that will point to GPU memory
	void* mapped_buffer_memory = nullptr;
};

struct STexture
{
	SBuffer transfer_buffer;
	unsigned int width;
	unsigned int height;
	VkFormat format;

	VkImage image;
	VkDeviceMemory memory;
	VkImageView view;
	VkImageLayout layout;
	VkSampler sampler;
};

struct SCamera
{
	glm::mat4 projection;
	glm::mat4 camera_position;
};

struct SParticleSystemSettings
{
	glm::vec3 system_position;					// In world space where is the particle system located
	float delta_time;
	float max_life;
};

// Since the data when passed as a storage buffer needs to be in multiples of 16 bytes
struct SParticleData
{
public:
	SParticleData() {};
	SParticleData(glm::vec4 position, glm::vec4 color, glm::vec3 velocity) : pos(position), color(color), velocity(velocity){}
	glm::vec4 pos;
	glm::vec4 color;
	glm::vec3 velocity;
	float life;
};

// Each particle will be made up of one point in space and turned into a quad by the geometry shader
SParticleData particle_verticies[PARTICLE_COUNT];

// How big will the buffer be in bytes
VkDeviceSize particle_vertex_buffer_size = sizeof(SParticleData) * PARTICLE_COUNT;

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

const unsigned int shader_stage_count = 3;
std::unique_ptr<VkShaderModule> graphics_shader_modules;
VkPipeline graphics_pipeline = VK_NULL_HANDLE;
VkPipelineLayout graphics_pipeline_layout = VK_NULL_HANDLE;

std::unique_ptr<VkFence> compute_fence;
VkShaderModule compute_shader_module;
VkPipeline compute_pipeline = VK_NULL_HANDLE;
VkPipelineLayout compute_pipeline_layout = VK_NULL_HANDLE;
VkCommandBuffer compute_command_buffer = VK_NULL_HANDLE;


VkBuffer particle_vertex_buffer = VK_NULL_HANDLE;
VkDeviceMemory particle_vertex_buffer_memory = VK_NULL_HANDLE;
// Raw pointer that will point to GPU memory
void* particle_vertex_mapped_buffer_memory = nullptr;
VkDescriptorPool particle_descriptor_pool;
VkDescriptorSetLayout particle_descriptor_set_layout;
VkDescriptorSet particle_descriptor_set;


SCamera camera;
SBuffer camera_buffer;
VkDescriptorPool camera_descriptor_pool;
VkDescriptorSetLayout camera_descriptor_set_layout;
VkDescriptorSet camera_descriptor_set;

SParticleSystemSettings particle_system_settings;
SBuffer particle_system_buffer;
std::chrono::steady_clock::time_point last_frame_time;
float delta_time;

VkDescriptorPool texture_descriptor_pool;
VkDescriptorSetLayout texture_descriptor_set_layout;
VkDescriptorSet texture_descriptor_set;

VkDrawIndirectCommand indirect_draw_command;
SBuffer indirect_draw_buffer;

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

void BuildCamera()
{
	camera.camera_position = glm::mat4(1.0f);
	camera.camera_position = glm::translate(camera.camera_position, glm::vec3( 0.0f, -3.0f, -10.0f ));

	camera.projection = glm::perspective(
		glm::radians(45.0f),										// What is the field of view
		(float)window_width / (float)window_height,					// Aspect ratio
		0.1f,														// Close view plane
		100.0f														// Far view plane
	);
	// Flip the cameras prospective upside down as glm assumes that the renderer we are using renders top to bottom, vulkan is the opposite
	camera.projection[1][1] *= -1.0f;

	// Transfer data over to the texture buffer
	memcpy(
		camera_buffer.mapped_buffer_memory,         // The destination for our memory (GPU)
		&camera,                                    // Source for the memory (CPU-Ram)
		sizeof(SCamera)                             // How much data we are transferring
	);
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
				BuildCamera();
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

// Everything within the Setup is from previous tutorials
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


	// Define what Layers and Extensions we require
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
		"Compute Particle System", VK_MAKE_VERSION(1, 0, 0),
		"Vulkan", VK_MAKE_VERSION(1, 0, 0),
		VK_MAKE_VERSION(1, 1, 108));

	// Attach a debugger to the application to give us validation feedback.
	// This is useful as it tells us about any issues without application
	debugger = VkHelper::CreateDebugger(instance);

	// Define what Device Extensions we require
	const uint32_t physical_device_extention_count = 1;
	// Note that this extension list is different from the instance on as we are telling the system what device settings we need.
	const char *physical_device_extensions[physical_device_extention_count] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };



	// The surface creation is added here as it needs to happen before the physical device creation and after we have said to the instance that we need a surface
	// The surface is the reference back to the OS window
	CreateSurface();



	// Find a physical device for us to use
	bool foundPhysicalDevice = VkHelper::GetPhysicalDevice(
		instance, 
		physical_device,                                       // Return of physical device instance
		physical_device_properties,                            // Physical device properties
		physical_devices_queue_family,                         // Physical device queue family
		physical_device_features,                              // Physical device features
		physical_device_mem_properties,                        // Physical device memory properties
		physical_device_extensions,                            // What extensions out device needs to have
		physical_device_extention_count,                       // Extension count
		VK_QUEUE_GRAPHICS_BIT,                                 // What queues we need to be available
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
		physical_device_extensions,                            // What extensions do you want on the device
		physical_device_extention_count                        // How many extensions are there
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
		VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT        // Allows any commands we create, the ability to be reset. This is helpful as we wont need to
	);                                                         // keep allocating new commands, we can reuse them


	CreateRenderResources();



	// Create the fences required for the swap chain rendering 
	VkHelper::CreateFence(device, fences, swapchain_image_count);

	// Create the image available and finished semaphore
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

	// Create the models vertex buffer
	VkHelper::CreateBuffer(
		device,                                                                      // What device are we going to use to create the buffer
		physical_device_mem_properties,                                              // What memory properties are available on the device
		particle_vertex_buffer,                                                      // What buffer are we going to be creating
		particle_vertex_buffer_memory,                                               // The output for the buffer memory
		particle_vertex_buffer_size,                                                 // How much memory we wish to allocate on the GPU
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,      // What type of buffer do we want. Buffers can have multiple types, for example, uniform & vertex buffer.
																		             // for now we want to keep the buffer specialized to one type as this will allow vulkan to optimize the data.
		VK_SHARING_MODE_EXCLUSIVE,                                                   // There are two modes, exclusive and concurrent. Defines if it can concurrently be used by multiple queue
																		             // families at the same time
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT										     // What properties do we require of our memory
	);

	// Get the pointer to the GPU memory
	VkResult vertex_mapped_memory_result = vkMapMemory(
		device,                                                         // The device that the memory is on
		particle_vertex_buffer_memory,                                           // The device memory instance
		0,                                                              // Offset from the memory starts that we are accessing
		particle_vertex_buffer_size,                                             // How much memory are we accessing
		0,                                                              // Flags (we don't need this for basic buffers)
		&particle_vertex_mapped_buffer_memory                                    // The return for the memory pointer
	);

	// Could we map the GPU memory to our CPU accessible pointer
	assert(vertex_mapped_memory_result == VK_SUCCESS);


	// Define how many descriptor pools we will need
	const uint32_t descriptor_pool_size_count = 1;

	// Define that the new descriptor pool will be taking a combined image sampler
	VkDescriptorPoolSize texture_pool_size[descriptor_pool_size_count] = {
		VkHelper::DescriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1)
	};

	// Create the new descriptor pool with the defined settings
	texture_descriptor_pool = VkHelper::CreateDescriptorPool(
		device,
		texture_pool_size,
		descriptor_pool_size_count,
		1															// How many descriptor sets do will we need
																	// Since we are only rendering one image, just 1
	);

	// Define at what stage of the rendering pipeline the texture is coming into as well as what shader and what location
	VkDescriptorSetLayoutBinding layout_bindings[descriptor_pool_size_count] = {
		VkHelper::DescriptorSetLayoutBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT)
	};

	// Create the new texture set layout
	texture_descriptor_set_layout = VkHelper::CreateDescriptorSetLayout(
		device,
		layout_bindings,
		descriptor_pool_size_count
	);

	// Allocate a new texture descriptor set from the pool
	texture_descriptor_set = VkHelper::AllocateDescriptorSet(
		device,
		texture_descriptor_pool,
		texture_descriptor_set_layout,
		1
	);




	camera_buffer.buffer_size = sizeof(SCamera);
	camera_buffer.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	camera_buffer.sharing_mode = VK_SHARING_MODE_EXCLUSIVE;
	camera_buffer.buffer_memory_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

	// Create a temp buffer to store the image in before we transfer it over to the image
	VkHelper::CreateBuffer(
		device,								   // What device are we going to use to create the buffer
		physical_device_mem_properties,		   // What memory properties are available on the device
		camera_buffer.buffer,                  // What buffer are we going to be creating
		camera_buffer.buffer_memory,           // The output for the buffer memory
		camera_buffer.buffer_size,             // How much memory we wish to allocate on the GPU
		camera_buffer.usage,                   // What type of buffer do we want. Buffers can have multiple types, for example, uniform & vertex buffer.
											   // for now we want to keep the buffer specialized to one type as this will allow vulkan to optimize the data.
		camera_buffer.sharing_mode,            // There are two modes, exclusive and concurrent. Defines if it can concurrently be used by multiple queue
											   // families at the same time
		camera_buffer.buffer_memory_properties // What properties do we require of our memory
	);

	// Get the pointer to the GPU memory
	VkResult mapped_memory_result = vkMapMemory(
		device,                                        // The device that the memory is on
		camera_buffer.buffer_memory,                   // The device memory instance
		0,                                             // Offset from the memory starts that we are accessing
		camera_buffer.buffer_size,                     // How much memory are we accessing
		0,                                             // Flags (we don't need this for basic buffers)
		&camera_buffer.mapped_buffer_memory            // The return for the memory pointer
	);

	// Could we map the GPU memory to our CPU accessible pointer
	assert(mapped_memory_result == VK_SUCCESS);

	BuildCamera();

	// Define how many descriptor pools we will need
	const uint32_t camera_descriptor_pool_size_count = 1;

	// Define that the new descriptor pool will be taking a combined image sampler
	VkDescriptorPoolSize camera_pool_size[camera_descriptor_pool_size_count] = {
		VkHelper::DescriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1)
	};


	// Create the new descriptor pool with the defined settings
	camera_descriptor_pool = VkHelper::CreateDescriptorPool(
		device,
		camera_pool_size,
		camera_descriptor_pool_size_count,
		1															// How many descriptor sets do will we need
																	// Since we are only rendering one image, just 1
	);

	// Define at what stage of the rendering pipeline the texture is coming into as well as what shader and what location
	VkDescriptorSetLayoutBinding camera_layout_bindings[camera_descriptor_pool_size_count] = {
		VkHelper::DescriptorSetLayoutBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT)
	};

	// Create the new texture set layout
	camera_descriptor_set_layout = VkHelper::CreateDescriptorSetLayout(
		device,
		camera_layout_bindings,
		camera_descriptor_pool_size_count
	);

	// Allocate a new texture descriptor set from the pool
	camera_descriptor_set = VkHelper::AllocateDescriptorSet(
		device,
		camera_descriptor_pool,
		camera_descriptor_set_layout,
		1
	);
	

	particle_system_buffer.buffer_size = sizeof(SParticleSystemSettings);
	particle_system_buffer.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	particle_system_buffer.sharing_mode = VK_SHARING_MODE_EXCLUSIVE;
	particle_system_buffer.buffer_memory_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

	// Create a temp buffer to store the image in before we transfer it over to the image
	VkHelper::CreateBuffer(
		device,												// What device are we going to use to create the buffer
		physical_device_mem_properties,						// What memory properties are available on the device
		particle_system_buffer.buffer,						// What buffer are we going to be creating
		particle_system_buffer.buffer_memory,				// The output for the buffer memory
		particle_system_buffer.buffer_size,					// How much memory we wish to allocate on the GPU
		particle_system_buffer.usage,						// What type of buffer do we want. Buffers can have multiple types, for example, uniform & vertex buffer.
															// for now we want to keep the buffer specialized to one type as this will allow vulkan to optimize the data.
		particle_system_buffer.sharing_mode,				// There are two modes, exclusive and concurrent. Defines if it can concurrently be used by multiple queue
															// families at the same time
		particle_system_buffer.buffer_memory_properties		// What properties do we require of our memory
	);

	// Get the pointer to the GPU memory
	mapped_memory_result = vkMapMemory(
		device,                                        // The device that the memory is on
		particle_system_buffer.buffer_memory,          // The device memory instance
		0,                                             // Offset from the memory starts that we are accessing
		particle_system_buffer.buffer_size,            // How much memory are we accessing
		0,                                             // Flags (we don't need this for basic buffers)
		&particle_system_buffer.mapped_buffer_memory   // The return for the memory pointer
	);

	// Could we map the GPU memory to our CPU accessible pointer
	assert(mapped_memory_result == VK_SUCCESS);

	// Work out the time as the application starts so we can work out the next frames time
	last_frame_time = Clock::now();

	VkDescriptorBufferInfo descriptorImageInfo = VkHelper::DescriptorBufferInfo(
		camera_buffer.buffer,
		camera_buffer.buffer_size,
		0
	);

	VkWriteDescriptorSet descriptorWrite = VkHelper::WriteDescriptorSet(camera_descriptor_set, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, descriptorImageInfo, 0);

	// Update the descriptor with the texture data
	vkUpdateDescriptorSets(
		device,
		1,
		&descriptorWrite,
		0,
		NULL
	);

	// Define the various rendering settings that we normally do in the command buffer recording
	indirect_draw_command.firstInstance = 0;							// If we are rendering a range of models (which we are), whats the first model index we want to render?
	indirect_draw_command.firstVertex = 0;								// How far along the vertex buffer should we offset
	indirect_draw_command.instanceCount = 1;							// How many models are we wanting to render? Each model gets a position from the position
	indirect_draw_command.vertexCount = PARTICLE_COUNT;					// Normally the vertex count represents how many points of a triangle we want
																		// To render, in this instance as we are in point mode, or draw each point individually
																		// each particle is a vertex and will be expanded by the geometry shader into a billboard particle


	VkDeviceSize indirect_buffer_size = sizeof(VkDrawIndirectCommand);

	VkHelper::CreateBuffer(
		device,                                                          // What device are we going to use to create the buffer
		physical_device_mem_properties,                                  // What memory properties are available on the device
		indirect_draw_buffer.buffer,                                     // What buffer are we going to be creating
		indirect_draw_buffer.buffer_memory,                              // The output for the buffer memory
		indirect_buffer_size,                                            // How much memory we wish to allocate on the GPU
		VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,                             // What type of buffer do we want. Buffers can have multiple types, for example, uniform & vertex buffer.
																		 // for now we want to keep the buffer specialized to one type as this will allow vulkan to optimize the data.
		VK_SHARING_MODE_EXCLUSIVE,                                       // There are two modes, exclusive and concurrent. Defines if it can concurrently be used by multiple queue
																		 // families at the same time
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT                              // What properties do we require of our memory
	);

	// Get the pointer to the GPU memory
	mapped_memory_result = vkMapMemory(
		device,                                                         // The device that the memory is on
		indirect_draw_buffer.buffer_memory,                             // The device memory instance
		0,                                                              // Offset from the memory starts that we are accessing
		indirect_buffer_size,                                           // How much memory are we accessing
		0,                                                              // Flags (we don't need this for basic buffers)
		&indirect_draw_buffer.mapped_buffer_memory                      // The return for the memory pointer
	);

	// Could we map the GPU memory to our CPU accessible pointer
	assert(mapped_memory_result == VK_SUCCESS);



	// Transfer data over to the indirect buffer
	memcpy(
		indirect_draw_buffer.mapped_buffer_memory,                      // The destination for our memory (GPU)
		&indirect_draw_command,                                         // Source for the memory (CPU-Ram)
		indirect_buffer_size                                            // How much data we are transferring
	);

}

// Everything within the Destroy is from previous tutorials
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


	// Clean up descriptor set layout
	vkDestroyDescriptorSetLayout(
		device,
		texture_descriptor_set_layout,
		nullptr
	);

	// Clean up descriptor pool
	vkDestroyDescriptorPool(
		device,
		texture_descriptor_pool,
		nullptr
	);

	vkDestroyDescriptorPool(
		device,
		camera_descriptor_pool,
		nullptr
	);

	vkDestroyDescriptorSetLayout(
		device,
		camera_descriptor_set_layout,
		nullptr
	);

	// Now we unmap the data
	vkUnmapMemory(
		device,
		particle_vertex_buffer_memory
	);

	// Clean up the buffer data
	vkDestroyBuffer(
		device,
		particle_vertex_buffer,
		nullptr
	);

	// Free the memory that was allocated for the buffer
	vkFreeMemory(
		device,
		particle_vertex_buffer_memory,
		nullptr
	);

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
	// We cant directly call vkDestroyDebugReportCallbackEXT as we need to find the pointer within the Vulkan DLL, See function implementation for details.
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

void CreateGraphicsPipeline()
{
	// How many unique vertex input attributes are we passing, in this instance, we are passing a color and position
	const uint32_t vertex_input_attribute_description_count = 2;
	// Create the pointer array for the vertex input attachments
	std::unique_ptr<VkVertexInputAttributeDescription> vertex_input_attribute_descriptions =
		std::unique_ptr<VkVertexInputAttributeDescription>(new VkVertexInputAttributeDescription[vertex_input_attribute_description_count]);

	// Used to store the position data of the vertex
	vertex_input_attribute_descriptions.get()[0].binding = 0;								// What vertex input binding are we talking about
	vertex_input_attribute_descriptions.get()[0].location = 0;								// Inside the binding what is its location id
	vertex_input_attribute_descriptions.get()[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;	// What format is the data coming in as
	vertex_input_attribute_descriptions.get()[0].offset = offsetof(SParticleData, pos);	    // Within the whole structure of the data packet, where dose it start in memory

	// Used to store the color data of the vertex
	vertex_input_attribute_descriptions.get()[1].binding = 0;								// What vertex input binding are we talking about
	vertex_input_attribute_descriptions.get()[1].location = 1;								// Inside the binding what is its location id
	vertex_input_attribute_descriptions.get()[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;	// What format is the data coming in as
	vertex_input_attribute_descriptions.get()[1].offset = offsetof(SParticleData, color);	// Within the whole structure of the data packet, where dose it start in memory


	// How many unique vertex input buffers are we passing to the pipeline
	const uint32_t vertex_input_binding_description_count = 1;
	// Pointer array of all the vertex input buffer definitions
	std::unique_ptr<VkVertexInputBindingDescription> vertex_input_binding_descriptions =
		std::unique_ptr<VkVertexInputBindingDescription>(new VkVertexInputBindingDescription[vertex_input_binding_description_count]);

	vertex_input_binding_descriptions.get()[0].binding = 0;									// What binding number is the buffer, this must be the same
																							// for the vertex input attributes
	vertex_input_binding_descriptions.get()[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;		// How often should the data be given to the GPU
	vertex_input_binding_descriptions.get()[0].stride = sizeof(SParticleData);				// How big is each vertex element

	// Define the paths for the shaders
	const char* shader_paths[shader_stage_count]{
		"../../Data/Shaders/14-ComputeParticleSystem/ParticleSystemGraphics/vert.spv",
		"../../Data/Shaders/14-ComputeParticleSystem/ParticleSystemGraphics/geom.spv",
		"../../Data/Shaders/14-ComputeParticleSystem/ParticleSystemGraphics/frag.spv"
	};
	// Define what shader types it should expect
	VkShaderStageFlagBits shader_stages_bits[shader_stage_count]{
		VK_SHADER_STAGE_VERTEX_BIT,
		VK_SHADER_STAGE_GEOMETRY_BIT,
		VK_SHADER_STAGE_FRAGMENT_BIT
	};
	graphics_shader_modules = std::unique_ptr<VkShaderModule>(new VkShaderModule[shader_stage_count]);
	
	// How many states do we want the ability to dynamically update
	const uint32_t dynamic_state_count = 3;
	// What dynamic states do we want access to change in command buffer recording
	VkDynamicState dynamic_states[dynamic_state_count] = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR,
		VK_DYNAMIC_STATE_LINE_WIDTH
	};

	// How many descriptor sets do we want to attach to the pipeline
	const uint32_t descriptor_set_layout_count = 2;

	// Define the descriptor set layouts that will be used for the pipeline
	VkDescriptorSetLayout descriptor_set_layout[descriptor_set_layout_count] = {
		camera_descriptor_set_layout,
		texture_descriptor_set_layout
	};

	// Create the pipeline
	graphics_pipeline = VkHelper::CreateGraphicsPipeline(
		physical_device,
		device,
		renderpass,
		graphics_pipeline_layout,
		shader_stage_count,
		shader_paths,
		shader_stages_bits,
		graphics_shader_modules.get(),
		descriptor_set_layout_count,
		descriptor_set_layout,
		vertex_input_attribute_description_count,
		vertex_input_attribute_descriptions.get(),
		vertex_input_binding_description_count,
		vertex_input_binding_descriptions.get(),
		dynamic_state_count,
		dynamic_states,
		VK_PRIMITIVE_TOPOLOGY_POINT_LIST,
		VK_POLYGON_MODE_FILL,
		100.0f,
		VK_CULL_MODE_NONE
	);
	
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

void DestroyGraphicsPipeline()
{
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
	for (int i = 0; i < shader_stage_count; i++)
	{
		vkDestroyShaderModule(
			device,
			graphics_shader_modules.get()[i],
			nullptr
		);
	}
}

void CreateComputePipeline()
{

	// Define how many descriptor pools we will need
	const uint32_t vertex_descriptor_pool_size_count = 2;

	// Define that the new descriptor pool will be taking a combined image sampler
	VkDescriptorPoolSize vertex_pool_size[vertex_descriptor_pool_size_count] = {
		VkHelper::DescriptorPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1),
		VkHelper::DescriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1),
	};


	// Create the new descriptor pool with the defined settings
	particle_descriptor_pool = VkHelper::CreateDescriptorPool(
		device,
		vertex_pool_size,
		vertex_descriptor_pool_size_count,
		1															// How many descriptor sets do will we need
																	// Since we are only rendering one image, just 1
	);

	// Define at what stage of the rendering pipeline the texture is coming into as well as what shader and what location
	VkDescriptorSetLayoutBinding vertex_layout_bindings[vertex_descriptor_pool_size_count] = {
		VkHelper::DescriptorSetLayoutBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT),
		VkHelper::DescriptorSetLayoutBinding(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT)
	};

	// Create the new texture set layout
	particle_descriptor_set_layout = VkHelper::CreateDescriptorSetLayout(
		device,
		vertex_layout_bindings,
		vertex_descriptor_pool_size_count
	);

	// Allocate a new texture descriptor set from the pool
	particle_descriptor_set = VkHelper::AllocateDescriptorSet(
		device,
		particle_descriptor_pool,
		particle_descriptor_set_layout,
		1
	);

	{
		// Update the vertex buffer being accessed by the compute shader
		VkDescriptorBufferInfo vertex_descriptor_buffer_info = VkHelper::DescriptorBufferInfo(
			particle_vertex_buffer,
			particle_vertex_buffer_size,
			0
		);

		VkWriteDescriptorSet descriptor_writes = VkHelper::WriteDescriptorSet(
			particle_descriptor_set,
			VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			vertex_descriptor_buffer_info,
			0
		);


		// Update the descriptor with the texture data
		vkUpdateDescriptorSets(
			device,
			1,
			&descriptor_writes,
			0,
			NULL
		);
	}


	{
		// Update the vertex buffer being accessed by the compute shader
		VkDescriptorBufferInfo vertex_descriptor_buffer_info = VkHelper::DescriptorBufferInfo(
			particle_system_buffer.buffer,
			particle_system_buffer.buffer_size,
			0
		);

		VkWriteDescriptorSet descriptor_writes = VkHelper::WriteDescriptorSet(
			particle_descriptor_set,
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			vertex_descriptor_buffer_info,
			1
		);


		// Update the descriptor with the texture data
		vkUpdateDescriptorSets(
			device,
			1,
			&descriptor_writes,
			0,
			NULL
		);
	}


	const uint32_t descriptor_set_layout_count = 1;

	VkDescriptorSetLayout descriptor_set_layout[descriptor_set_layout_count] = {
		particle_descriptor_set_layout
	};

	compute_pipeline = VkHelper::CreateComputePipeline(
		device,
		compute_pipeline_layout,
		"../../Data/Shaders/14-ComputeParticleSystem/ParticleSystemCompute/comp.spv",
		compute_shader_module,
		descriptor_set_layout_count,
		descriptor_set_layout
	);


	compute_fence = std::unique_ptr<VkFence>();
	// Create a new fence to make sure the execution of the command buffer was successful 
	VkHelper::CreateFence(
		device,
		compute_fence,
		1
	);

	// Define the structure needed to allocate a new command buffer
	VkCommandBufferAllocateInfo command_buffer_allocate_info = VkHelper::CommandBufferAllocateInfo(
		command_pool,
		1
	);

	// Allocate the new command buffer
	VkResult allocate_command_buffer_resut = vkAllocateCommandBuffers(
		device,
		&command_buffer_allocate_info,
		&compute_command_buffer
	);
}

void DestroyComputePipeline()
{

	vkDestroyFence(
		device,
		*compute_fence.get(),
		nullptr
	);

	vkDestroyPipeline(
		device,
		compute_pipeline,
		nullptr
	);

	vkDestroyPipelineLayout(
		device,
		compute_pipeline_layout,
		nullptr
	);

	vkDestroyShaderModule(
		device,
		compute_shader_module,
		nullptr
	);
}

STexture CreateTexture(const char* path)
{
	STexture texture;
	// Used to store the raw image data
	std::vector<unsigned char> image_data;

	// Load using "loadpng" the image from file
	unsigned error = lodepng::decode(
		image_data,
		texture.width,
		texture.height,
		path
	);
	// Validate that the image could be found and accessed
	if (error)
	{
		std::cout << lodepng_error_text(error) << std::endl;
		exit(-1);
	}

	// How big is the texture in bytes? RGBA = 4 bytes * Width * Height
	const unsigned int texture_size = texture.width * texture.height * 4;


	texture.transfer_buffer.buffer_size = texture_size;
	texture.transfer_buffer.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	texture.transfer_buffer.sharing_mode = VK_SHARING_MODE_EXCLUSIVE;
	texture.transfer_buffer.buffer_memory_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

	// Create a temp buffer to store the image in before we transfer it over to the image
	VkHelper::CreateBuffer(
		device,											 // What device are we going to use to create the buffer
		physical_device_mem_properties,					 // What memory properties are available on the device
		texture.transfer_buffer.buffer,                  // What buffer are we going to be creating
		texture.transfer_buffer.buffer_memory,           // The output for the buffer memory
		texture.transfer_buffer.buffer_size,             // How much memory we wish to allocate on the GPU
		texture.transfer_buffer.usage,                   // What type of buffer do we want. Buffers can have multiple types, for example, uniform & vertex buffer.
														 // for now we want to keep the buffer specialized to one type as this will allow vulkan to optimize the data.
		texture.transfer_buffer.sharing_mode,            // There are two modes, exclusive and concurrent. Defines if it can concurrently be used by multiple queue
														 // families at the same time
		texture.transfer_buffer.buffer_memory_properties // What properties do we require of our memory
	);

	// Get the pointer to the GPU memory
	VkResult mapped_memory_result = vkMapMemory(
		device,                                                  // The device that the memory is on
		texture.transfer_buffer.buffer_memory,                   // The device memory instance
		0,                                                       // Offset from the memory starts that we are accessing
		texture.transfer_buffer.buffer_size,                     // How much memory are we accessing
		0,                                                       // Flags (we don't need this for basic buffers)
		&texture.transfer_buffer.mapped_buffer_memory            // The return for the memory pointer
	);

	// Could we map the GPU memory to our CPU accessible pointer
	assert(mapped_memory_result == VK_SUCCESS);


	// Transfer data over to the texture buffer
	memcpy(
		texture.transfer_buffer.mapped_buffer_memory,         // The destination for our memory (GPU)
		image_data.data(),                                    // Source for the memory (CPU-Ram)
		texture_size                                          // How much data we are transferring
	);


	// Now that the texture data is on the graphics card, we can unmap it
	vkUnmapMemory(
		device,
		texture.transfer_buffer.buffer_memory
	);

	texture.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;		// Define that the the image is read only
																	// This makes image reads from the shader faster
	texture.format = VK_FORMAT_R8G8B8A8_UNORM;						// What is the format of the image, in this case, 4 bytes RGBA


	// Create the image we will be rendering
	VkHelper::CreateImage(
		device,
		physical_device_mem_properties,
		texture.width,
		texture.height,
		texture.format,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		texture.image,
		texture.memory,
		VK_IMAGE_LAYOUT_UNDEFINED										// Here we define the initial layout, later we change its layout to VK_FORMAT_R8G8B8A8_UNORM
	);

	// Create a new command that we will run on the GPU, this will be used to change the images settings
	// and transfer the image data between the buffer and the image
	VkCommandBuffer copy_cmd = VkHelper::BeginSingleTimeCommands(device, command_pool);

	// The sub resource range describes the regions of the image we will be transition
	VkImageSubresourceRange subresourceRange = {};
	// Image only contains color data
	subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	// Start at first mip level
	subresourceRange.baseMipLevel = 0;
	// We will transition on all mip levels
	subresourceRange.levelCount = 1;
	// The 2D texture only has one layer
	subresourceRange.layerCount = 1;

	// Optimal image will be used as destination for the copy, so we must transfer from our
	// initial undefined image layout to the transfer destination layout

	VkHelper::SetImageLayout(
		copy_cmd,
		texture.image,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		subresourceRange
	);

	// Only dealing with one mip level for now
	VkBufferImageCopy bufferCopyRegion = {};
	bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	bufferCopyRegion.imageSubresource.mipLevel = 0;
	bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
	bufferCopyRegion.imageSubresource.layerCount = 1;
	bufferCopyRegion.imageExtent.width = static_cast<uint32_t>(texture.width);
	bufferCopyRegion.imageExtent.height = static_cast<uint32_t>(texture.height);
	bufferCopyRegion.imageExtent.depth = 1;
	bufferCopyRegion.bufferOffset = 0;

	// Copy mip levels from staging buffer
	vkCmdCopyBufferToImage(
		copy_cmd,
		texture.transfer_buffer.buffer,
		texture.image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1,
		&bufferCopyRegion
	);



	// Now the data is mapped to the texture, we no longer need the buffer
	vkDestroyBuffer(
		device,
		texture.transfer_buffer.buffer,
		nullptr
	);

	// Now change the images layout from VK_IMAGE_LAYOUT_UNDEFINED to VK_FORMAT_R8G8B8A8_UNORM
	VkHelper::SetImageLayout(
		copy_cmd,
		texture.image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		texture.layout,							// Set to VK_FORMAT_R8G8B8A8_UNORM
		subresourceRange
	);

	// Finish the new command and run it
	VkHelper::EndSingleTimeCommands(
		device,
		graphics_queue,
		copy_cmd,
		command_pool
	);

	// Clean up after the command buffer
	vkFreeCommandBuffers(
		device,
		command_pool,
		1,
		&copy_cmd
	);


	// Create a new image sampler, this will allow the shaders to sample the texture
	VkHelper::CreateImageSampler(
		device,
		texture.image,
		texture.format,
		texture.view,
		texture.sampler
	);

	return texture;
}

void DestroyTexture(STexture& texture)
{
	// Free the memory that was allocated for the buffer
	vkFreeMemory(
		device,
		texture.transfer_buffer.buffer_memory,
		nullptr
	);

	// Destroy the image sampler now that we are done with that
	vkDestroySampler(
		device,
		texture.sampler,
		nullptr
	);

	// Destroy the image we are displaying
	vkDestroyImage(
		device,
		texture.image,
		nullptr
	);
}

void SetAlbedoTexture(STexture& texture)
{
	VkDescriptorImageInfo descriptorImageInfo = VkHelper::DescriptorImageInfo(
		texture.sampler,
		texture.view,
		texture.layout
	);

	VkWriteDescriptorSet descriptorWrite = VkHelper::WriteDescriptorSet(texture_descriptor_set, descriptorImageInfo, 0);

	// Update the descriptor with the texture data
	vkUpdateDescriptorSets(
		device,
		1,
		&descriptorWrite,
		0,
		NULL
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
		
		VkDeviceSize offsets[] = { 0 };
		// Bind the models vertex buffer
		vkCmdBindVertexBuffers(
			command_buffers.get()[i],
			0,
			1,
			&particle_vertex_buffer,
			offsets
		);


		// Bind the position to the pipeline
		vkCmdBindDescriptorSets(
			command_buffers.get()[i],
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			graphics_pipeline_layout,
			0,
			1,
			&camera_descriptor_set,
			0,
			NULL
		);

		// Bind the texture to the pipeline
		vkCmdBindDescriptorSets(
			command_buffers.get()[i],
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			graphics_pipeline_layout,
			1,
			1,
			&texture_descriptor_set,
			0,
			NULL
		);

		// Render from the pre made render buffer 
		vkCmdDrawIndirect(
			command_buffers.get()[i],
			indirect_draw_buffer.buffer,
			0,
			1,
			sizeof(VkDrawIndirectCommand)
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

void BuildComputeBuffers()
{
	// Create a info structure to start the command buffer recording
	VkCommandBufferBeginInfo command_buffer_begin_info = VkHelper::CommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT);

	// Start the command buffer recording process
	VkResult begin_command_buffer_result = vkBeginCommandBuffer(
		compute_command_buffer,
		&command_buffer_begin_info
	);

	// Attach the pipeline to the command
	vkCmdBindPipeline(
		compute_command_buffer,
		VK_PIPELINE_BIND_POINT_COMPUTE,
		compute_pipeline
	);

	// Bind the data descriptor set to the command
	vkCmdBindDescriptorSets(
		compute_command_buffer,
		VK_PIPELINE_BIND_POINT_COMPUTE,
		compute_pipeline_layout,
		0,
		1,
		&particle_descriptor_set,
		0,
		0
	);

	// Execute the command
	// The way the compute commands are ran is in a 3D coordinate space, for example, if we wanted to process a texture
	// we would want to run the compute shader for the width x height of the texture, but in this instance we just want 
	// to process it on a 1D array, so to work out the total amount of shader calls X * Y * Z or 'data_size' * 1 * 1
	vkCmdDispatch(
		compute_command_buffer,
		PARTICLE_COUNT,							// X axis
		1,										// Y axis
		1										// Z axis
	);

	// End the command recording
	VkResult end_command_buffer_result = vkEndCommandBuffer(
		compute_command_buffer
	);

	assert(end_command_buffer_result == VK_SUCCESS);
}

void UpdateParticleSystem()
{
	std::chrono::steady_clock::time_point current_time = Clock::now();

	float time_fraction = ((float)std::chrono::duration_cast<std::chrono::nanoseconds>(current_time - last_frame_time).count() / (float)1000000000);

	delta_time += time_fraction;

	last_frame_time = current_time;

	particle_system_settings.max_life = PARTICLE_MAX_LIFE;				// How long we want each particle to live for
	particle_system_settings.delta_time = time_fraction;

	particle_system_settings.system_position = glm::vec3( sin(delta_time * 0.25f) * 4, 0, 0 );

	// Transfer data over to the texture buffer
	memcpy(
		particle_system_buffer.mapped_buffer_memory,		// The destination for our memory (GPU)
		&particle_system_settings,							// Source for the memory (CPU-Ram)
		sizeof(SParticleSystemSettings)						// How much data we are transferring
	);


	// Create a new submit info instance
	VkSubmitInfo submit_info = VkHelper::SubmitInfo(compute_command_buffer);

	// Submit the command pool to the queue for execution
	VkResult compute_submit_result = vkQueueSubmit(
		graphics_queue,										// You can use the graphics queue to process compute tasks
		1,
		&submit_info,
		*compute_fence.get()
	);
	assert(compute_submit_result == VK_SUCCESS);

	// Wait for the fence to be released, this will happen upon the completion of the compute shader
	VkResult wait_for_fence_result = vkWaitForFences(
		device,
		1,
		compute_fence.get(),
		VK_TRUE,
		LONG_MAX
	);
	assert(wait_for_fence_result == VK_SUCCESS);

	// Reset the fence for the "next" call of the shader, in this case we are not doing multiple calls, but we could
	VkResult reset_fence_result = vkResetFences(
		device,
		1,
		compute_fence.get()
	);
	assert(wait_for_fence_result == VK_SUCCESS);
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
	// Setup the vulkan instance and device settings
	Setup();

	// Create the graphics pipeline
	CreateGraphicsPipeline();

	// Create the particle system compute pipeline
	CreateComputePipeline();

	// Create albedo texture for the model
	STexture metal_texture = CreateTexture("../../Data/Images/eye.png");

	// Set the albedo texture
	SetAlbedoTexture(metal_texture);

	///////////////////////////////////////////
	///// Init The Particle System Buffer /////
	///////////////////////////////////////////



	for (int i = 0; i < PARTICLE_COUNT; i++)
	{
		particle_verticies[i].life = (1.0f / PARTICLE_COUNT) * i * PARTICLE_MAX_LIFE + PARTICLE_MAX_LIFE;
		particle_verticies[i].color = glm::vec4(1.0f);
		particle_verticies[i].velocity = glm::normalize(glm::vec3(
			((rand() % 2000) - 1000) * 0.001f,
			1.0f,
			0.0f
		));
	}

	// Transfer particles over to the GPU
	memcpy(
		particle_vertex_mapped_buffer_memory,                                         // The destination for our memory (GPU)
		particle_verticies,								                             // Source for the memory (CPU-Ram)
		sizeof(SParticleData) * PARTICLE_COUNT				                     // How much data we are transferring
	);

	// Regenerate the command buffers
	BuildCommandBuffers(graphics_command_buffers, swapchain_image_count);

	BuildComputeBuffers();

	while (window_open)
	{
		PollWindow();

		UpdateParticleSystem();
		Render();
	}

	////////////////////
	///// Clean Up /////
	////////////////////


	// Now we unmap the data
	vkUnmapMemory(
		device,
		indirect_draw_buffer.buffer_memory
	);

	// Clean up the buffer data
	vkDestroyBuffer(
		device,
		indirect_draw_buffer.buffer,
		nullptr
	);

	// Free the memory that was allocated for the buffer
	vkFreeMemory(
		device,
		indirect_draw_buffer.buffer_memory,
		nullptr
	);

	DestroyTexture(metal_texture);

	DestroyComputePipeline();

	DestroyGraphicsPipeline();

	// Finish previous projects cleanups
	Destroy();

	return 0;
}