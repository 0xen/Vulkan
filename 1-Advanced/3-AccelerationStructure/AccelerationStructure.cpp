
#define VK_USE_PLATFORM_WIN32_KHR
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <memory>
#include <stdexcept>
#include <iostream>


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

class SVertexData
{
public:
	SVertexData() {};
	SVertexData(glm::vec3 position, glm::vec3 normal, glm::vec3 color, glm::vec2 uv, unsigned int materialID) : pos(position), texCoord(uv), nrm(normal), color(color), matID(materialID) {}
	glm::vec3 pos;
	glm::vec3 nrm;
	glm::vec3 color;
	glm::vec2 texCoord;
	unsigned int matID;
};

struct ModelInstance
{
	unsigned int vertex_offset = 0;
	unsigned int index_offset = 0;
	unsigned int vertex_count = 0;
	unsigned int index_count = 0;

	VkGeometryNV geometry;

	VkAccelerationStructureNV acceleration_structure = VK_NULL_HANDLE;


	VkBuffer scratch_buffer = VK_NULL_HANDLE;
	VkDeviceMemory scratch_buffer_memory = VK_NULL_HANDLE;
	// Raw pointer that will point to GPU memory
	void* scratch_mapped_buffer_memory = nullptr;

	VkBuffer result_buffer = VK_NULL_HANDLE;
	VkDeviceMemory result_buffer_memory = VK_NULL_HANDLE;
	// Raw pointer that will point to GPU memory
	void* result_mapped_buffer_memory = nullptr;
};


struct ASInstance
{
	VkAccelerationStructureNV bottomLevelAS;

	const glm::mat4x4 transform;

	// Instance Id that is visible to the shader
	uint32_t instanceID;

	// Hit group index used to fetch the shaders from the SBT
	uint32_t hitGroupIndex;
};

ModelInstance pbrboy_model;


const unsigned int verticies_count = 500000;
const unsigned int index_count = 500000;

unsigned int used_verticies = 0;
unsigned int used_index = 0;

VkDeviceSize particle_vertex_buffer_size = sizeof(SVertexData) * verticies_count;
VkDeviceSize index_buffer_size = sizeof(uint32_t) * index_count;

void CreateRenderResources();
void DestroyRenderResources();
void RebuildRenderResources();
void BuildCommandBuffers(std::unique_ptr<VkCommandBuffer>& command_buffers, const uint32_t buffer_count);

VkInstance instance;
VkDebugReportCallbackEXT debugger;
VkPhysicalDevice physical_device = VK_NULL_HANDLE;


uint32_t physical_devices_queue_family = 0;
VkPhysicalDeviceRayTracingPropertiesNV device_raytracing_properties = {};
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



VkAccelerationStructureNV top_level_acceleration_structure = VK_NULL_HANDLE;

// The AS needs some space to store temporary info, this space requirement is dependent on the scene complexity
VkDeviceSize top_level_as_scratch_size = 0;

// We need to calculate the final AS size
VkDeviceSize top_level_as_result_size = 0;

// Required GPU memory to store instance
VkDeviceSize top_level_as_instances_size = 0;

VkBuffer top_level_as_scratch_buffer = VK_NULL_HANDLE;
VkDeviceMemory top_level_as_scratch_buffer_memory = VK_NULL_HANDLE;
// Raw pointer that will point to GPU memory
void* top_level_as_scratch_mapped_buffer_memory = nullptr;

VkBuffer top_level_as_result_buffer = VK_NULL_HANDLE;
VkDeviceMemory top_level_as_result_buffer_memory = VK_NULL_HANDLE;
// Raw pointer that will point to GPU memory
void* top_level_as_result_mapped_buffer_memory = nullptr;

VkBuffer top_level_as_instance_buffer = VK_NULL_HANDLE;
VkDeviceMemory top_level_as_instance_buffer_memory = VK_NULL_HANDLE;
// Raw pointer that will point to GPU memory
void* top_level_as_instance_mapped_buffer_memory = nullptr;

uint32_t current_frame_index = 0; // What frame are we currently using
std::unique_ptr<VkFence> fences = nullptr;

VkSemaphore image_available_semaphore;
VkSemaphore render_finished_semaphore;
VkSubmitInfo render_submission_info = {};
VkPresentInfoKHR present_info = {};
VkPipelineStageFlags wait_stages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

std::unique_ptr<VkCommandBuffer> graphics_command_buffers = nullptr;


VkBuffer particle_vertex_buffer = VK_NULL_HANDLE;
VkDeviceMemory particle_vertex_buffer_memory = VK_NULL_HANDLE;
// Raw pointer that will point to GPU memory
void* particle_vertex_mapped_buffer_memory = nullptr;


VkBuffer index_buffer = VK_NULL_HANDLE;
VkDeviceMemory index_buffer_memory = VK_NULL_HANDLE;
// Raw pointer that will point to GPU memory
void* index_mapped_buffer_memory = nullptr;

SCamera camera;
SBuffer camera_buffer;
VkDescriptorPool camera_descriptor_pool;
VkDescriptorSetLayout camera_descriptor_set_layout;
VkDescriptorSet camera_descriptor_set;

VkDescriptorPool texture_descriptor_pool;
VkDescriptorSetLayout texture_descriptor_set_layout;
VkDescriptorSet texture_descriptor_set;


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
	camera.camera_position = glm::translate(camera.camera_position, glm::vec3(0.0f, -0.07f, -0.2f));

	camera.camera_position = glm::inverse(camera.camera_position);

	camera.projection = glm::perspective(
		glm::radians(45.0f),										// What is the field of view
		(float)window_width / (float)window_height,					// Aspect ratio
		0.1f,														// Close view plane
		100.0f														// Far view plane
	);

	// Flip the cameras prospective upside down as glm assumes that the renderer we are using renders top to bottom, vulkan is the opposite
	camera.projection[1][1] *= -1.0f;
	camera.projection = glm::inverse(camera.projection);

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
void Setup()
{

	WindowSetup("Vulkan", 1080, 720);


	// Define what Layers and Extensions we require
	const uint32_t extention_count = 4;
	const char *instance_extensions[extention_count] = { VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME, "VK_EXT_debug_report" ,VK_KHR_SURFACE_EXTENSION_NAME,VK_KHR_WIN32_SURFACE_EXTENSION_NAME };
	const uint32_t layer_count = 1;
	const char *instance_layers[layer_count] = { "VK_LAYER_LUNARG_standard_validation" };

	// Check to see if we have the layer requirments
	assert(VkHelper::CheckLayersSupport(instance_layers, 1) && "Unsupported Layers Found");

	// Create the Vulkan Instance
	instance = VkHelper::CreateInstance(
		instance_extensions, extention_count,
		instance_layers, layer_count,
		"Raytracing", VK_MAKE_VERSION(1, 0, 0),
		"Vulkan", VK_MAKE_VERSION(1, 0, 0),
		VK_MAKE_VERSION(1, 1, 108));

	// Attach a debugger to the application to give us validation feedback.
	// This is useful as it tells us about any issues without application
	debugger = VkHelper::CreateDebugger(instance);

	// Define what Device Extensions we require
	const uint32_t physical_device_extention_count = 3;
	// Note that this extension list is different from the instance on as we are telling the system what device settings we need.
	const char *physical_device_extensions[physical_device_extention_count] = { 
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		VK_NV_RAY_TRACING_EXTENSION_NAME,
		VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME
	};



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
		device,																		// What device are we going to use to create the buffer
		physical_device_mem_properties,												// What memory properties are available on the device
		particle_vertex_buffer,														// What buffer are we going to be creating
		particle_vertex_buffer_memory,												// The output for the buffer memory
		particle_vertex_buffer_size,												// How much memory we wish to allocate on the GPU
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,		// What type of buffer do we want. Buffers can have multiple types, for example, uniform & vertex buffer.
																					// for now we want to keep the buffer specialized to one type as this will allow vulkan to optimize the data.
		VK_SHARING_MODE_EXCLUSIVE,													// There are two modes, exclusive and concurrent. Defines if it can concurrently be used by multiple queue
																					// families at the same time
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT											// What properties do we require of our memory
	);

	// Get the pointer to the GPU memory
	VkResult vertex_mapped_memory_result = vkMapMemory(
		device,																		// The device that the memory is on
		particle_vertex_buffer_memory,												// The device memory instance
		0,																			// Offset from the memory starts that we are accessing
		particle_vertex_buffer_size,												// How much memory are we accessing
		0,																			// Flags (we don't need this for basic buffers)
		&particle_vertex_mapped_buffer_memory										// The return for the memory pointer
	);

	// Could we map the GPU memory to our CPU accessible pointer
	assert(vertex_mapped_memory_result == VK_SUCCESS);

	// Create the models Index buffer
	VkHelper::CreateBuffer(
		device,																		// What device are we going to use to create the buffer
		physical_device_mem_properties,												// What memory properties are available on the device
		index_buffer,																// What buffer are we going to be creating
		index_buffer_memory,														// The output for the buffer memory
		index_buffer_size,															// How much memory we wish to allocate on the GPU
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,      // What type of buffer do we want. Buffers can have multiple types, for example, uniform & vertex buffer.
																					// for now we want to keep the buffer specialized to one type as this will allow vulkan to optimize the data.
		VK_SHARING_MODE_EXCLUSIVE,													// There are two modes, exclusive and concurrent. Defines if it can concurrently be used by multiple queue
																					// families at the same time
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT											// What properties do we require of our memory
	);

	// Get the pointer to the GPU memory
	VkResult index_mapped_memory_result = vkMapMemory(
		device,																		// The device that the memory is on
		index_buffer_memory,														// The device memory instance
		0,																			// Offset from the memory starts that we are accessing
		index_buffer_size,															// How much memory are we accessing
		0,																			// Flags (we don't need this for basic buffers)
		&index_mapped_buffer_memory													// The return for the memory pointer
	);

	// Could we map the GPU memory to our CPU accessible pointer
	assert(index_mapped_memory_result == VK_SUCCESS);



	// Define how many descriptor pools we will need
	const uint32_t descriptor_pool_size_count = 3;

	// Define that the new descriptor pool will be taking a combined image sampler
	VkDescriptorPoolSize texture_pool_size[descriptor_pool_size_count] = {
		VkHelper::DescriptorPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1),
		VkHelper::DescriptorPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1),
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
		VkHelper::DescriptorSetLayoutBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV),
		VkHelper::DescriptorSetLayoutBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV),
		VkHelper::DescriptorSetLayoutBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV)
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


	
	VkDescriptorBufferInfo descriptorVertexInfo = VkHelper::DescriptorBufferInfo(
		particle_vertex_buffer,
		particle_vertex_buffer_size,
		0
	);

	VkDescriptorBufferInfo descriptorIndexInfo = VkHelper::DescriptorBufferInfo(
		index_buffer,
		index_buffer_size,
		0
	);

	VkWriteDescriptorSet descriptorModelWrites[2];
	descriptorModelWrites[0] = VkHelper::WriteDescriptorSet(texture_descriptor_set, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, descriptorVertexInfo, 0);
	descriptorModelWrites[1] = VkHelper::WriteDescriptorSet(texture_descriptor_set, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, descriptorIndexInfo, 1);

	// Update the descriptor with the texture data
	vkUpdateDescriptorSets(
		device,
		2,
		descriptorModelWrites,
		0,
		NULL
	);
	






	camera_buffer.buffer_size = sizeof(SCamera);
	camera_buffer.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
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
	const uint32_t camera_descriptor_pool_size_count = 3;

	// Define that the new descriptor pool will be taking a combined image sampler
	VkDescriptorPoolSize camera_pool_size[camera_descriptor_pool_size_count] = {
		VkHelper::DescriptorPoolSize(VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV, 1),
		VkHelper::DescriptorPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1),
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
		VkHelper::DescriptorSetLayoutBinding(0, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV, 1, VK_SHADER_STAGE_RAYGEN_BIT_NV),
		VkHelper::DescriptorSetLayoutBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_RAYGEN_BIT_NV),
		VkHelper::DescriptorSetLayoutBinding(2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_RAYGEN_BIT_NV | VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV | VK_SHADER_STAGE_MISS_BIT_NV)
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



	VkDescriptorBufferInfo descriptorCameraInfo = VkHelper::DescriptorBufferInfo(
		camera_buffer.buffer,
		camera_buffer.buffer_size,
		0
	);

	VkWriteDescriptorSet descriptorCameraWrite = VkHelper::WriteDescriptorSet(
		camera_descriptor_set,
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		descriptorCameraInfo,
		2
	);

	// Update the descriptor with the texture data
	vkUpdateDescriptorSets(
		device,
		1,
		&descriptorCameraWrite,
		0,
		NULL
	);

}

// Everything within the Destroy is from previous tutorials
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


STexture CreateTexture(unsigned int width, unsigned int height, VkImageUsageFlags usageFlags)
{
	STexture texture;
	texture.width = width;
	texture.height = height;

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


	texture.layout = VK_IMAGE_LAYOUT_GENERAL;						
																	// This makes image reads from the shader faster
	texture.format = VK_FORMAT_B8G8R8A8_UNORM;						// What is the format of the image, in this case, 4 bytes RGBA


	// Create the image we will be rendering
	VkHelper::CreateImage(
		device,
		physical_device_mem_properties,
		texture.width,
		texture.height,
		texture.format,
		VK_IMAGE_TILING_OPTIMAL,
		usageFlags,
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

	VkWriteDescriptorSet descriptorWrite = VkHelper::WriteDescriptorSet(texture_descriptor_set, descriptorImageInfo, 2);

	// Update the descriptor with the texture data
	vkUpdateDescriptorSets(
		device,
		1,
		&descriptorWrite,
		0,
		NULL
	);
}

void CreateModel(const char* path, ModelInstance& model_instance)
{
	
	ObjLoader<SVertexData> loader;
	loader.loadModel(path);

	model_instance.vertex_offset = used_verticies;
	model_instance.index_offset = used_index;
	model_instance.vertex_count = loader.m_vertices.size();
	model_instance.index_count = loader.m_indices.size();



	// First we copy the example data to the GPU
	memcpy(
		index_mapped_buffer_memory,                                         // The destination for our memory (GPU)
		loader.m_indices.data(),                                            // Source for the memory (CPU-Ram)
		model_instance.index_count * sizeof(uint32_t)                       // How much data we are transferring
	);

	// First we copy the example data to the GPU
	memcpy(
		particle_vertex_mapped_buffer_memory,                                         // The destination for our memory (GPU)
		loader.m_vertices.data(),                                            // Source for the memory (CPU-Ram)
		model_instance.vertex_count * sizeof(SVertexData)                     // How much data we are transferring
	);


	used_verticies += model_instance.vertex_count;
	used_index += model_instance.index_count;

	// Build bottom level acceleration structure
	model_instance.geometry = VkHelper::CreateRayTraceGeometry(
		particle_vertex_buffer,													// buffer
		model_instance.vertex_offset * sizeof(SVertexData),						// Offset
		model_instance.vertex_count,											// Size
		sizeof(SVertexData),													// Each vertex size

		index_buffer,															// Index buffer
		model_instance.index_offset * sizeof(uint32_t),							// Index offset
		model_instance.index_count,												// Get Index Size

		VK_NULL_HANDLE, 0, true);
}

void DestroyRenderResources()
{
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

		// Begin the new command buffer
		VkResult begin_command_buffer_result = vkBeginCommandBuffer(
			command_buffers.get()[i],
			&command_buffer_begin_info
		);
		// check to see if the command buffer was created
		assert(begin_command_buffer_result == VK_SUCCESS);

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




		// We will add the raytracing pipline and descriptors down the line




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

VKAPI_ATTR VkResult VKAPI_CALL
vkCreateRayTracingPipelinesNV(VkDevice      device,
	VkPipelineCache                         pipelineCache,
	uint32_t                                createInfoCount,
	const VkRayTracingPipelineCreateInfoNV* pCreateInfos,
	const VkAllocationCallbacks* pAllocator,
	VkPipeline* pPipelines)
{
	static const auto call = reinterpret_cast<PFN_vkCreateRayTracingPipelinesNV>(
		vkGetDeviceProcAddr(device, "vkCreateRayTracingPipelinesNV"));
	return call(device, pipelineCache, createInfoCount, pCreateInfos, pAllocator, pPipelines);
}

VKAPI_ATTR VkResult VKAPI_CALL
vkCreateAccelerationStructureNV(VkDevice       device,
	const VkAccelerationStructureCreateInfoNV* pCreateInfo,
	const VkAllocationCallbacks* pAllocator,
	VkAccelerationStructureNV* pAccelerationStructure)
{
	static const auto call = reinterpret_cast<PFN_vkCreateAccelerationStructureNV>(
		vkGetDeviceProcAddr(device, "vkCreateAccelerationStructureNV"));
	return call(device, pCreateInfo, pAllocator, pAccelerationStructure);
}

VKAPI_ATTR void VKAPI_CALL vkGetAccelerationStructureMemoryRequirementsNV(
	VkDevice                                               device,
	const VkAccelerationStructureMemoryRequirementsInfoNV* pInfo,
	VkMemoryRequirements2KHR* pMemoryRequirements)
{
	static const auto call = reinterpret_cast<PFN_vkGetAccelerationStructureMemoryRequirementsNV>(
		vkGetDeviceProcAddr(device, "vkGetAccelerationStructureMemoryRequirementsNV"));
	return call(device, pInfo, pMemoryRequirements);
}


VKAPI_ATTR VkResult VKAPI_CALL
vkBindAccelerationStructureMemoryNV(VkDevice       device,
	uint32_t                                       bindInfoCount,
	const VkBindAccelerationStructureMemoryInfoNV* pBindInfos)
{
	static const auto call = reinterpret_cast<PFN_vkBindAccelerationStructureMemoryNV>(
		vkGetDeviceProcAddr(device, "vkBindAccelerationStructureMemoryNV"));
	return call(device, bindInfoCount, pBindInfos);
}

VKAPI_ATTR void VKAPI_CALL
vkCmdBuildAccelerationStructureNV(
	VkCommandBuffer                      commandBuffer,
	const VkAccelerationStructureInfoNV* pInfo,
	VkBuffer                             instanceData,
	VkDeviceSize                         instanceOffset,
	VkBool32                             update,
	VkAccelerationStructureNV            dst,
	VkAccelerationStructureNV            src,
	VkBuffer                             scratch,
	VkDeviceSize                         scratchOffset)
{
	static const auto call = reinterpret_cast<PFN_vkCmdBuildAccelerationStructureNV>(
		vkGetDeviceProcAddr(device, "vkCmdBuildAccelerationStructureNV"));
	return call(commandBuffer, pInfo, instanceData, instanceOffset, update, dst, src, scratch,
		scratchOffset);
}

VKAPI_ATTR VkResult VKAPI_CALL
vkGetAccelerationStructureHandleNV(VkDevice device,
	VkAccelerationStructureNV accelerationStructure,
	size_t                    dataSize,
	void* pData)
{
	static const auto call = reinterpret_cast<PFN_vkGetAccelerationStructureHandleNV>(
		vkGetDeviceProcAddr(device, "vkGetAccelerationStructureHandleNV"));
	return call(device, accelerationStructure, dataSize, pData);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetRayTracingShaderGroupHandlesNV(VkDevice   device,
	VkPipeline pipeline,
	uint32_t   firstGroup,
	uint32_t   groupCount,
	size_t     dataSize,
	void* pData)
{
	static const auto call = reinterpret_cast<PFN_vkGetRayTracingShaderGroupHandlesNV>(
		vkGetDeviceProcAddr(device, "vkGetRayTracingShaderGroupHandlesNV"));
	return call(device, pipeline, firstGroup, groupCount, dataSize, pData);
}

VKAPI_ATTR void VKAPI_CALL vkCmdTraceRaysNV(VkCommandBuffer commandBuffer,
	VkBuffer        raygenShaderBindingTableBuffer,
	VkDeviceSize    raygenShaderBindingOffset,
	VkBuffer        missShaderBindingTableBuffer,
	VkDeviceSize    missShaderBindingOffset,
	VkDeviceSize    missShaderBindingStride,
	VkBuffer        hitShaderBindingTableBuffer,
	VkDeviceSize    hitShaderBindingOffset,
	VkDeviceSize    hitShaderBindingStride,
	VkBuffer        callableShaderBindingTableBuffer,
	VkDeviceSize    callableShaderBindingOffset,
	VkDeviceSize    callableShaderBindingStride,
	uint32_t        width,
	uint32_t        height,
	uint32_t        depth)
{
	static const auto call = reinterpret_cast<PFN_vkCmdTraceRaysNV>(
		vkGetDeviceProcAddr(device, "vkCmdTraceRaysNV"));
	return call(commandBuffer, raygenShaderBindingTableBuffer, raygenShaderBindingOffset,
		missShaderBindingTableBuffer, missShaderBindingOffset, missShaderBindingStride,
		hitShaderBindingTableBuffer, hitShaderBindingOffset, hitShaderBindingStride,
		callableShaderBindingTableBuffer, callableShaderBindingOffset,
		callableShaderBindingStride, width, height, depth);
}

VkDeviceSize CopyShaderData(uint8_t* outputData, uint32_t groupIndex, VkDeviceSize entrySize, const uint8_t* shaderHandleStorage, unsigned char* inlineData, unsigned int inlineDataSize)
{
	VkDeviceSize progIdSize = device_raytracing_properties.shaderGroupHandleSize;

	// Copy the shader identifier that was previously obtained with
	memcpy(outputData, shaderHandleStorage + groupIndex * progIdSize, progIdSize);

	// Copy all its resources pointers or values in bulk
	if (inlineDataSize > 0)
	{
		memcpy(outputData + progIdSize, inlineData, inlineDataSize);
	}

	// Return the number of bytes actually written to the output buffer
	return entrySize;
}

struct VkGeometryInstance
{
	/// Transform matrix, containing only the top 3 rows
	float transform[12];
	/// Instance index
	uint32_t instanceId : 24;
	/// Visibility mask
	uint32_t mask : 8;
	/// Index of the hit group which will be invoked when a ray hits the instance
	uint32_t instanceOffset : 24;
	/// Instance flags, such as culling
	uint32_t flags : 8;
	/// Opaque handle of the bottom-level acceleration structure
	uint64_t accelerationStructureHandle;
};

void BuildAccelerationStructure(VkCommandBuffer commandBuffer, VkAccelerationStructureInfoNV& acceleration_structure_info, VkBuffer instance_buffer,
	VkAccelerationStructureNV& acceleration_structure, VkBuffer& scratch_buffer)
{
	vkCmdBuildAccelerationStructureNV(commandBuffer, &acceleration_structure_info, instance_buffer, 0, VK_FALSE,
		acceleration_structure, VK_NULL_HANDLE, scratch_buffer, 0);

	// Ensure that the build will be finished before using the AS using a barrier
	VkMemoryBarrier memoryBarrier;
	memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
	memoryBarrier.pNext = nullptr;
	memoryBarrier.srcAccessMask =
		VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NV;
	memoryBarrier.dstAccessMask =
		VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NV;

	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV,
		VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV, 0, 1, &memoryBarrier,
		0, nullptr, 0, nullptr);
}

void CreateBottomLevelASBuffer(VkCommandBuffer commandBuffer, ModelInstance& model)
{
	{
		VkAccelerationStructureInfoNV acceleration_structure_info = VkHelper::AccelerationStructureInfoNV(
			VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_NV,
			0,
			&model.geometry,
			1,
			0
		);

		VkAccelerationStructureCreateInfoNV create_info = VkHelper::AccelerationStructureCreateInfoNV(acceleration_structure_info);


		VkResult create_acceleration_structure_result = vkCreateAccelerationStructureNV(
			device,
			&create_info,
			nullptr,
			&model.acceleration_structure
		);
	}

	// The AS needs some space to store temporary info, this space requirements is dependent on the scene complexity
	VkDeviceSize scratch_size = 0;

	// We need to calculate the final AS size
	VkDeviceSize result_size = 0;

	VkAccelerationStructureMemoryRequirementsInfoNV memory_requirments_info = VkHelper::AccelerationStructureMemoryRequirmentsInfoNV(model.acceleration_structure);

	VkMemoryRequirements2 memoryRequirements;

	{
		vkGetAccelerationStructureMemoryRequirementsNV(
			device,
			&memory_requirments_info,
			&memoryRequirements
		);

		// Size of the resulting AS
		result_size = memoryRequirements.memoryRequirements.size;
	}

	{
		// Store the memory requirements
		memory_requirments_info.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_BUILD_SCRATCH_NV;
		vkGetAccelerationStructureMemoryRequirementsNV(
			device,
			&memory_requirments_info,
			&memoryRequirements
		);

		scratch_size = memoryRequirements.memoryRequirements.size;
	}

	{
		memory_requirments_info.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_UPDATE_SCRATCH_NV;
		vkGetAccelerationStructureMemoryRequirementsNV(
			device,
			&memory_requirments_info,
			&memoryRequirements
		);

		scratch_size = scratch_size > memoryRequirements.memoryRequirements.size ? scratch_size : memoryRequirements.memoryRequirements.size;

	}

	{
		VkHelper::CreateBuffer(
			device,																// What device are we going to use to create the buffer
			physical_device_mem_properties,										// What memory properties are available on the device
			model.scratch_buffer,														// What buffer are we going to be creating
			model.scratch_buffer_memory,												// The output for the buffer memory
			scratch_size,														// How much memory we wish to allocate on the GPU
			VK_BUFFER_USAGE_RAY_TRACING_BIT_NV,				                    // What type of buffer do we want. Buffers can have multiple types, for example, uniform & vertex buffer.
																				// for now we want to keep the buffer specialized to one type as this will allow vulkan to optimize the data.
			VK_SHARING_MODE_EXCLUSIVE,						                    // There are two modes, exclusive and concurrent. Defines if it can concurrently be used by multiple queue
																				// families at the same time
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT									// What properties do we require of our memory
		);
	}

	{
		VkHelper::CreateBuffer(
			device,																// What device are we going to use to create the buffer
			physical_device_mem_properties,										// What memory properties are available on the device
			model.result_buffer,														// What buffer are we going to be creating
			model.result_buffer_memory,												// The output for the buffer memory
			result_size,														// How much memory we wish to allocate on the GPU
			VK_BUFFER_USAGE_RAY_TRACING_BIT_NV,				                    // What type of buffer do we want. Buffers can have multiple types, for example, uniform & vertex buffer.
																				// for now we want to keep the buffer specialized to one type as this will allow vulkan to optimize the data.
			VK_SHARING_MODE_EXCLUSIVE,						                    // There are two modes, exclusive and concurrent. Defines if it can concurrently be used by multiple queue
																				// families at the same time
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT									// What properties do we require of our memory
		);
	}




	// Generate the bottom level AS
	{

		VkBindAccelerationStructureMemoryInfoNV acceleration_memory_info = VkHelper::AccelerationStructureMemoryInfoNV(
			model.acceleration_structure, model.result_buffer_memory);


		VkResult code = vkBindAccelerationStructureMemoryNV(
			device,
			1,
			&acceleration_memory_info
		);
		assert(code == VK_SUCCESS);

		VkAccelerationStructureInfoNV acceleration_structure_info = VkHelper::AccelerationStructureInfo(0, model.geometry);

		BuildAccelerationStructure(commandBuffer, acceleration_structure_info, VK_NULL_HANDLE, model.acceleration_structure, model.scratch_buffer);


	}
}

void CreateTopLevelASBuffer(VkCommandBuffer commandBuffer, unsigned int model_instances_count)
{

	// Create Top Level AS Instance

	VkBuildAccelerationStructureFlagsNV flags = VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_NV;

	VkAccelerationStructureInfoNV acceleration_structure_info = VkHelper::AccelerationStructureInfoNV(
		VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_NV,
		flags,
		VK_NULL_HANDLE,
		0,
		model_instances_count // Total AS count
	);

	VkAccelerationStructureCreateInfoNV create_info = VkHelper::AccelerationStructureCreateInfoNV(acceleration_structure_info);


	VkResult create_acceleration_structure_result = vkCreateAccelerationStructureNV(
		device,
		&create_info,
		nullptr,
		&top_level_acceleration_structure
	);

	assert(create_acceleration_structure_result == VK_SUCCESS);



	VkAccelerationStructureMemoryRequirementsInfoNV memory_requirments_info = VkHelper::AccelerationStructureMemoryRequirmentsInfoNV(top_level_acceleration_structure);

	VkMemoryRequirements2 memoryRequirements;


	{
		vkGetAccelerationStructureMemoryRequirementsNV(device, &memory_requirments_info, &memoryRequirements);

		// Size of the resulting AS
		top_level_as_result_size = memoryRequirements.memoryRequirements.size;
	}

	{
		// Store the memory requirments
		memory_requirments_info.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_BUILD_SCRATCH_NV;
		vkGetAccelerationStructureMemoryRequirementsNV(device, &memory_requirments_info, &memoryRequirements);

		top_level_as_scratch_size = memoryRequirements.memoryRequirements.size;
	}

	{
		memory_requirments_info.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_UPDATE_SCRATCH_NV;
		vkGetAccelerationStructureMemoryRequirementsNV(device, &memory_requirments_info, &memoryRequirements);

		top_level_as_scratch_size = top_level_as_scratch_size > memoryRequirements.memoryRequirements.size ? top_level_as_scratch_size : memoryRequirements.memoryRequirements.size;

	}



	top_level_as_instances_size = model_instances_count * sizeof(VkGeometryInstance);


	{
		VkHelper::CreateBuffer(
			device,																// What device are we going to use to create the buffer
			physical_device_mem_properties,										// What memory properties are available on the device
			top_level_as_scratch_buffer,														// What buffer are we going to be creating
			top_level_as_scratch_buffer_memory,												// The output for the buffer memory
			top_level_as_scratch_size,														// How much memory we wish to allocate on the GPU
			VK_BUFFER_USAGE_RAY_TRACING_BIT_NV,				                    // What type of buffer do we want. Buffers can have multiple types, for example, uniform & vertex buffer.
																				// for now we want to keep the buffer specialized to one type as this will allow vulkan to optimize the data.
			VK_SHARING_MODE_EXCLUSIVE,						                    // There are two modes, exclusive and concurrent. Defines if it can concurrently be used by multiple queue
																				// families at the same time
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT									// What properties do we require of our memory
		);
	}

	{
		VkHelper::CreateBuffer(
			device,																// What device are we going to use to create the buffer
			physical_device_mem_properties,										// What memory properties are available on the device
			top_level_as_result_buffer,														// What buffer are we going to be creating
			top_level_as_result_buffer_memory,												// The output for the buffer memory
			top_level_as_result_size,														// How much memory we wish to allocate on the GPU
			VK_BUFFER_USAGE_RAY_TRACING_BIT_NV,				                    // What type of buffer do we want. Buffers can have multiple types, for example, uniform & vertex buffer.
																				// for now we want to keep the buffer specialized to one type as this will allow vulkan to optimize the data.
			VK_SHARING_MODE_EXCLUSIVE,						                    // There are two modes, exclusive and concurrent. Defines if it can concurrently be used by multiple queue
																				// families at the same time
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT									// What properties do we require of our memory
		);
	}



	if (top_level_as_instances_size > 0)
	{
		VkHelper::CreateBuffer(
			device,																		// What device are we going to use to create the buffer
			physical_device_mem_properties,												// What memory properties are available on the device
			top_level_as_instance_buffer,															// What buffer are we going to be creating
			top_level_as_instance_buffer_memory,														// The output for the buffer memory
			top_level_as_instances_size,																// How much memory we wish to allocate on the GPU
			VK_BUFFER_USAGE_RAY_TRACING_BIT_NV,											// What type of buffer do we want. Buffers can have multiple types, for example, uniform & vertex buffer.
																						// for now we want to keep the buffer specialized to one type as this will allow vulkan to optimize the data.
			VK_SHARING_MODE_EXCLUSIVE,													// There are two modes, exclusive and concurrent. Defines if it can concurrently be used by multiple queue
																						// families at the same time
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT	// What properties do we require of our memory
		);
	}

	// Bind top level memory on creation
	VkBindAccelerationStructureMemoryInfoNV bindInfo = VkHelper::AccelerationStructureMemoryInfoNV(top_level_acceleration_structure, top_level_as_result_buffer_memory);

	VkResult bind_acceleration_structure_result = vkBindAccelerationStructureMemoryNV(
		device,
		1,
		&bindInfo
	);

	assert(bind_acceleration_structure_result == VK_SUCCESS);

}

unsigned int UpdateGeometryInstances(ASInstance* model_instances, unsigned int model_instances_count)
{
	// Generate top level
	std::vector<VkGeometryInstance> geometryInstances;

	for(int i = 0 ; i < model_instances_count; i++)
	{
		const ASInstance& instance = model_instances[i];

		uint64_t accelerationStructureHandle = 0;
		VkResult code = vkGetAccelerationStructureHandleNV(
			device,
			instance.bottomLevelAS,
			sizeof(uint64_t),
			&accelerationStructureHandle
		);
		if (code != VK_SUCCESS)
		{
			throw std::logic_error("vkGetAccelerationStructureHandleNV failed");
		}

		VkGeometryInstance gInst;
		glm::mat4x4 transp = glm::transpose(instance.transform);
		memcpy(gInst.transform, &transp, sizeof(gInst.transform));
		gInst.instanceId = instance.instanceID;
		// The visibility mask is always set of 0xFF, but if some instances would need to be ignored in
		// some cases, this flag should be passed by the application
		gInst.mask = 0xff;
		// Set the hit group index, that will be used to find the shader code to execute when hitting
		// the geometry
		gInst.instanceOffset = instance.hitGroupIndex;

		// Disable culling
		gInst.flags = 0;// VK_GEOMETRY_INSTANCE_TRIANGLE_FRONT_COUNTERCLOCKWISE_BIT_NV;
		gInst.accelerationStructureHandle = accelerationStructureHandle;
		geometryInstances.push_back(gInst);
	}


	if (top_level_as_instances_size > 0)
	{
		VkResult mapped_memory_result = vkMapMemory(
			device,													// The device that the memory is on
			top_level_as_instance_buffer_memory,					// The device memory instance
			0,														// Offset from the memory starts that we are accessing
			top_level_as_instances_size,							// How much memory are we accessing
			0,														// Flags (we don't need this for basic buffers)
			&top_level_as_instance_mapped_buffer_memory				// The return for the memory pointer
		);

		memcpy(top_level_as_instance_mapped_buffer_memory, geometryInstances.data(), top_level_as_instances_size);

		vkUnmapMemory(
			device,
			top_level_as_instance_buffer_memory
		);
	}

	return geometryInstances.size();
}

int main(int argc, char **argv)
{

	Setup();



	// Create albedo texture for the model
	STexture metal_texture = CreateTexture("../../../Data/Images/futuristic-panels1-bl/futuristic-panels1-albedo.png");

	// Set the albedo texture
	SetAlbedoTexture(metal_texture);

	// Load the new model
	CreateModel("../../../Data/Models/PBRBoy.obj", pbrboy_model);








	device_raytracing_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PROPERTIES_NV;
	device_raytracing_properties.pNext = nullptr;
	device_raytracing_properties.maxRecursionDepth = 0;
	device_raytracing_properties.shaderGroupHandleSize = 0;


	VkPhysicalDeviceProperties2 physical_device_properties2 = {};
	physical_device_properties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
	physical_device_properties2.pNext = &device_raytracing_properties;
	physical_device_properties2.properties = {};

	VkPhysicalDeviceDescriptorIndexingFeaturesEXT descIndexFeatures;
	VkPhysicalDeviceFeatures2 device_features2;

	vkGetPhysicalDeviceProperties2(
		physical_device,
		&physical_device_properties2
	);

	descIndexFeatures = {};
	descIndexFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT;

	device_features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	device_features2.pNext = &descIndexFeatures;

	vkGetPhysicalDeviceFeatures2(
		physical_device,
		&device_features2
	);



	VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
	VkCommandBufferAllocateInfo command_buffer_allocate_info = VkHelper::CommandBufferAllocateInfo(
		command_pool,
		1
	);

	VkResult allocate_command_buffer_resut = vkAllocateCommandBuffers(
		device,
		&command_buffer_allocate_info,
		&commandBuffer
	);

	VkCommandBufferBeginInfo command_buffer_begin_info = VkHelper::CommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	// Begin the new command buffer
	VkResult begin_command_buffer_result = vkBeginCommandBuffer(
		commandBuffer,
		&command_buffer_begin_info
	);

	assert(begin_command_buffer_result == VK_SUCCESS);

	// Create the acceleration structure that will store our model instance
	CreateBottomLevelASBuffer(commandBuffer, pbrboy_model);


	std::vector<ASInstance> as_instance;

	as_instance.push_back({ 
		pbrboy_model.acceleration_structure,
		glm::mat4(1.0f), 
		0, 
		0												// What hit group is used on model ray collision
	});

	
	CreateTopLevelASBuffer(commandBuffer, as_instance.size());

	unsigned int geometryInstanceCount = UpdateGeometryInstances(as_instance.data(), as_instance.size());

	VkAccelerationStructureInfoNV acceleration_structure_info = VkHelper::AccelerationStructureInfo(0, geometryInstanceCount);
	acceleration_structure_info.flags = 1;

	BuildAccelerationStructure(commandBuffer, acceleration_structure_info, top_level_as_instance_buffer, top_level_acceleration_structure, top_level_as_scratch_buffer);

	// End the command buffer and submit it
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submit_info = {};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.pNext = nullptr;
	submit_info.waitSemaphoreCount = 0;
	submit_info.pWaitSemaphores = nullptr;
	submit_info.pWaitDstStageMask = nullptr;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &commandBuffer;
	submit_info.signalSemaphoreCount = 0;
	submit_info.pSignalSemaphores = nullptr;

	VkResult queue_submit_result = vkQueueSubmit(
		graphics_queue,
		1,
		&submit_info,
		VK_NULL_HANDLE
	);
	assert(queue_submit_result == VK_SUCCESS);

	VkResult queue_wait_result = vkQueueWaitIdle(graphics_queue);

	assert(queue_wait_result == VK_SUCCESS);

	vkFreeCommandBuffers(
		device,
		command_pool,
		1,
		&commandBuffer
	);

	VkWriteDescriptorSetAccelerationStructureNV acceleration_structure_definition = VkHelper::WriteDescriptorSetAccelerator(
		top_level_acceleration_structure
	);

	{ // Bind the acceleration structure to the descriptor set
		VkWriteDescriptorSet descriptorWrite = VkHelper::WriteDescriptorSet(
			camera_descriptor_set,
			VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV,
			acceleration_structure_definition,
			0
		);

		// Update the descriptor with the texture data
		vkUpdateDescriptorSets(
			device,
			1,
			&descriptorWrite,
			0,
			NULL
		);
	}


	// Regenerate the command buffers
	BuildCommandBuffers(graphics_command_buffers, swapchain_image_count);

	while (window_open)
	{
		PollWindow();

		Render();
	}

	////////////////////
	///// Clean Up ///// 
	////////////////////

	DestroyTexture(metal_texture);

	// Finish previous projects cleanups
	Destroy();

	return 0;
}