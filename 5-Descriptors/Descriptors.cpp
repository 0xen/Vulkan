#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <memory>

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
	// Define what Layers and Extentions we require
	const uint32_t extention_count = 1;
	const char *instance_extensions[extention_count] = { "VK_EXT_debug_report" };
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
		VK_QUEUE_GRAPHICS_BIT                                  // What queues we need to be avaliable
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


int main(int argc, char **argv)
{
	// Setup the components from the previous projects
	Setup();


	VkDescriptorPool descriptor_pool;
	VkDescriptorSetLayout descriptor_set_layout;
	VkDescriptorSet descriptor_set;
	VkWriteDescriptorSet descriptor_write_set;

	{ // Create Descriptor Pool
		VkDescriptorPoolSize pool_size = {};
		pool_size.type = VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		pool_size.descriptorCount = 1;

		VkDescriptorPoolCreateInfo create_info = {};
		create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		create_info.poolSizeCount = 1;
		create_info.pPoolSizes = &pool_size;
		create_info.maxSets = 100;

		VkResult create_descriptor_pool = vkCreateDescriptorPool(
			device,
			&create_info,
			nullptr,
			&descriptor_pool
		);


		assert(create_descriptor_pool == VK_SUCCESS);
	}

	{ // Create Descriptor Pool layout
		VkDescriptorSetLayoutBinding layout_bindings = {};
		layout_bindings.binding = 0;
		layout_bindings.descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		layout_bindings.descriptorCount = 1;
		layout_bindings.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;


		VkDescriptorSetLayoutCreateInfo layout_info = {};
		layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layout_info.bindingCount = 1;
		layout_info.pBindings = &layout_bindings;


		VkResult create_descriptor_set_layout = vkCreateDescriptorSetLayout(
			device,
			&layout_info,
			nullptr,
			&descriptor_set_layout
		);

		assert(create_descriptor_set_layout == VK_SUCCESS);
	}

	// Create descriptor set from descriptor pool
	{
		VkDescriptorSetAllocateInfo alloc_info = {};
		alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		alloc_info.descriptorPool = descriptor_pool;
		alloc_info.descriptorSetCount = 1;
		alloc_info.pSetLayouts = &descriptor_set_layout;

		VkResult create_descriptor_set = vkAllocateDescriptorSets(
			device,
			&alloc_info,
			&descriptor_set
		);

		assert(create_descriptor_set == VK_SUCCESS);
	}


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

		vkUpdateDescriptorSets(device,
			1, // Passing over 1 buffer
			&descriptor_write_set,
			0, 
			NULL
		);
	}






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


	// Finish previous projects cleanups
	Destroy();

	return 0;
}