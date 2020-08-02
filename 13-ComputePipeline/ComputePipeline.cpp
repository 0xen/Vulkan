
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
#include <vulkan/vulkan.h>

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

VkInstance instance;
VkDebugReportCallbackEXT debugger;
VkPhysicalDevice physical_device = VK_NULL_HANDLE;


uint32_t physical_devices_queue_family = 0;
VkPhysicalDeviceProperties physical_device_properties;
VkPhysicalDeviceFeatures physical_device_features;
VkPhysicalDeviceMemoryProperties physical_device_mem_properties;

VkDevice device = VK_NULL_HANDLE;
VkQueue compute_queue = VK_NULL_HANDLE;
VkCommandPool command_pool;



// Everything within the Setup is from previous tutorials
// Setup
// - Instance
// - Debugger
// - Physical Device
// - Device
// - Command Pool
// - Buffer
// - Descriptor
void Setup()
{


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
		"Compute Pipeline", VK_MAKE_VERSION(1, 0, 0),
		"Vulkan", VK_MAKE_VERSION(1, 0, 0),
		VK_MAKE_VERSION(1, 1, 108));

	// Attach a debugger to the application to give us validation feedback.
	// This is useful as it tells us about any issues without application
	debugger = VkHelper::CreateDebugger(instance);

	// Define what Device Extensions we require
	const uint32_t physical_device_extention_count = 1;
	// Note that this extension list is different from the instance on as we are telling the system what device settings we need.
	const char *physical_device_extensions[physical_device_extention_count] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };


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
		VK_QUEUE_COMPUTE_BIT                                   // What queues we need to be available
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
		&compute_queue
	);

	command_pool = VkHelper::CreateCommandPool(
		device,
		physical_devices_queue_family,                         // What queue family we are wanting to use to send commands to the GPU
		VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT        // Allows any commands we create, the ability to be reset. This is helpful as we wont need to
	);                                                         // keep allocating new commands, we can reuse them

}

// Everything within the Destroy is from previous tutorials
// Destroy
// - Descriptor
// - Buffer
// - Command Pool
// - Device
// - Debugger
// - Instance
void Destroy()
{
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
}


int main(int argc, char **argv)
{

	Setup();


	VkShaderModule compute_shader = VK_NULL_HANDLE;
	VkPipelineLayout compute_pipeline_layout = VK_NULL_HANDLE;
	VkPipeline compute_pipeline = VK_NULL_HANDLE;
	VkCommandBuffer compute_command_buffer = VK_NULL_HANDLE;


	////////////////////////////////////////////////
	///// Create Data To Manipulate On The GPU /////
	////////////////////////////////////////////////

	const unsigned int data_size = 10;
	// Create a array of floats that we will manipulate on the GPU
	float compute_data[data_size];

	// Loop through the array elements and populate them
	for (int i = 0; i < data_size; i++)
	{
		compute_data[i] = (float)i;
	}

	// Define the properties of the buffer
	SBuffer data_buffer = {};
	data_buffer.buffer_size = sizeof(float) * data_size;
	data_buffer.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	data_buffer.sharing_mode = VK_SHARING_MODE_EXCLUSIVE;
	data_buffer.buffer_memory_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

	// Create a temp buffer to store the image in before we transfer it over to the image
	VkHelper::CreateBuffer(
		device,								   // What device are we going to use to create the buffer
		physical_device_mem_properties,		   // What memory properties are available on the device
		data_buffer.buffer,                    // What buffer are we going to be creating
		data_buffer.buffer_memory,             // The output for the buffer memory
		data_buffer.buffer_size,               // How much memory we wish to allocate on the GPU
		data_buffer.usage,                     // What type of buffer do we want. Buffers can have multiple types, for example, uniform & vertex buffer.
											   // for now we want to keep the buffer specialized to one type as this will allow vulkan to optimize the data.
		data_buffer.sharing_mode,              // There are two modes, exclusive and concurrent. Defines if it can concurrently be used by multiple queue
											   // families at the same time
		data_buffer.buffer_memory_properties   // What properties do we require of our memory
	);

	// Get the pointer to the GPU memory
	VkResult mapped_memory_result = vkMapMemory(
		device,                                        // The device that the memory is on
		data_buffer.buffer_memory,                     // The device memory instance
		0,                                             // Offset from the memory starts that we are accessing
		data_buffer.buffer_size,                       // How much memory are we accessing
		0,                                             // Flags (we don't need this for basic buffers)
		&data_buffer.mapped_buffer_memory              // The return for the memory pointer
	);

	// Could we map the GPU memory to our CPU accessible pointer
	assert(mapped_memory_result == VK_SUCCESS);

	// Transfer data over to the buffer
	memcpy(
		data_buffer.mapped_buffer_memory,              // The destination for our memory (GPU)
		compute_data,                                  // Source for the memory (CPU-Ram)
		data_buffer.buffer_size                        // How much data we are transferring
	);



	///////////////////////////////////
	///// Create Compute Pipeline /////
	///////////////////////////////////


	// Define how many descriptor pools we will need
	const uint32_t data_descriptor_pool_size_count = 1;

	// Define that the new descriptor pool will be taking a combined image sampler
	VkDescriptorPoolSize data_pool_size[data_descriptor_pool_size_count] = {
		VkHelper::DescriptorPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1)
	};


	// Create the new descriptor pool with the defined settings
	VkDescriptorPool data_descriptor_pool = VkHelper::CreateDescriptorPool(
		device,
		data_pool_size,
		data_descriptor_pool_size_count,
		1															// How many descriptor sets do will we need
																	// Since we are only rendering one image, just 1
	);

	// Define at what stage of the rendering pipeline the texture is coming into as well as what shader and what location
	VkDescriptorSetLayoutBinding data_layout_bindings[data_descriptor_pool_size_count] = {
		VkHelper::DescriptorSetLayoutBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT)
	};

	// Create the new texture set layout
	VkDescriptorSetLayout data_descriptor_set_layout = VkHelper::CreateDescriptorSetLayout(
		device,
		data_layout_bindings,
		data_descriptor_pool_size_count
	);

	// Allocate a new texture descriptor set from the pool
	VkDescriptorSet data_descriptor_set = VkHelper::AllocateDescriptorSet(
		device,
		data_descriptor_pool,
		data_descriptor_set_layout,
		1
	);

	// Prepare the buffer definition that we want to update
	VkDescriptorBufferInfo descriptor_Info = VkHelper::DescriptorBufferInfo(
		data_buffer.buffer,
		data_buffer.buffer_size,
		0
	);

	// Define the descriptor type that we will be updating
	VkWriteDescriptorSet descriptorWrite = VkHelper::WriteDescriptorSet(data_descriptor_set, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, descriptor_Info, 0);

	// Update the descriptor with the texture data
	vkUpdateDescriptorSets(
		device,
		1,
		&descriptorWrite,
		0,
		NULL
	);

	// Define the new pipeline layout that we will be creating
	VkPipelineLayoutCreateInfo pipeline_layout_info = {};
	pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipeline_layout_info.setLayoutCount = 1;
	pipeline_layout_info.pSetLayouts = &data_descriptor_set_layout;
	pipeline_layout_info.pushConstantRangeCount = 0;
	pipeline_layout_info.pPushConstantRanges = 0;


	// Create the new pipeline layout
	VkResult create_pipeline_result = vkCreatePipelineLayout(
		device,
		&pipeline_layout_info,
		nullptr,
		&compute_pipeline_layout
	);

	// Was the pipeline layout created correctly
	assert(create_pipeline_result == VK_SUCCESS);

	
	char* shader_data = nullptr;
	unsigned int shader_size = 0;

	// Load the shader from file
	VkHelper::ReadShaderFile(
		"../../Data/Shaders/13-ComputePipeline/comp.spv",
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
		&compute_shader
	);
	// Validate the shader module
	assert(create_shader_module == VK_SUCCESS);

	// Destroy the CPU side memory
	delete[] shader_data;

	// Define the type of shader and what function should be called within the shader
	VkPipelineShaderStageCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	info.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	info.module = compute_shader;
	info.pName = "main";


	// Create the info structure needed to finalize the creation of the pipeline
	VkComputePipelineCreateInfo compute_pipeline_create_info{};
	compute_pipeline_create_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	compute_pipeline_create_info.layout = compute_pipeline_layout;
	compute_pipeline_create_info.stage = info;

	// Create the compute pipeline
	VkResult create_compute_pipeline = vkCreateComputePipelines(
		device,
		0,
		1,
		&compute_pipeline_create_info,
		nullptr,
		&compute_pipeline
	);

	assert(create_compute_pipeline == VK_SUCCESS);



	////////////////////////////////////////////////////////////////////////
	///// Record The Command Buffer That Will Execute The Compute Code /////
	////////////////////////////////////////////////////////////////////////

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

	assert(allocate_command_buffer_resut == VK_SUCCESS);

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
		&data_descriptor_set,
		0,
		0
	);

	// Execute the command
	// The way the compute commands are ran is in a 3D coordinate space, for example, if we wanted to process a texture
	// we would want to run the compute shader for the width x height of the texture, but in this instance we just want 
	// to process it on a 1D array, so to work out the total amount of shader calls X * Y * Z or 'data_size' * 1 * 1
	vkCmdDispatch(
		compute_command_buffer,
		data_size,								// X axis
		1,										// Y axis
		1										// Z axis
	);

	// End the command recording
	VkResult end_command_buffer_result = vkEndCommandBuffer(
		compute_command_buffer
	);

	assert(end_command_buffer_result == VK_SUCCESS);


	//////////////////////////////////
	///// Run The Compute Shader /////
	//////////////////////////////////


	std::unique_ptr<VkFence> compute_fence = std::unique_ptr<VkFence>();
	// Create a new fence to make sure the execution of the command buffer was successful 
	VkHelper::CreateFence(
		device,
		compute_fence,
		1
	);

	// Create a new submit info instance
	VkSubmitInfo submit_info = VkHelper::SubmitInfo(compute_command_buffer);

	// Submit the command pool to the queue for execution
	VkResult compute_submit_result = vkQueueSubmit(
		compute_queue,
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




	///////////////////////////////////////////////
	///// Output The Result Of The Processing /////
	///////////////////////////////////////////////


	std::cout << "Data Before GPU Processing" << std::endl;
	for (int i = 0; i < data_size; i++)
	{
		std::cout << compute_data[i] << std::endl;
	}

	// Transfer back from the GPU to CPU memory
	memcpy(
		compute_data,                                  // The destination for our memory (CPU-Ram)
		data_buffer.mapped_buffer_memory,              // Source for the memory (GPU)
		data_buffer.buffer_size                        // How much data we are transferring
	);

	std::cout << "Data After GPU Calculated Data x4" << std::endl;
	for (int i = 0; i < data_size; i++)
	{
		std::cout << compute_data[i] << std::endl;
	}



	////////////////////
	///// Clean Up /////
	////////////////////




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
		compute_shader,
		nullptr
	);

	vkDestroyFence(
		device,
		*compute_fence.get(),
		nullptr
	);

	// Finish previous projects cleanups
	Destroy();

	return 0;
}