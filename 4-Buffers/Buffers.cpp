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


// Everything within the Setup is from previous tuturials
// Setup
// - Instance
// - Debugger
// - Physical Device
// - Device
// - Command Pool
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
}

// Everything within the Destroy is from previous tuturials
// Destroy
// - Debugger
// - Instance
// - Device
// - Command Pool
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


uint32_t FindMemoryType(uint32_t type_filter, VkMemoryPropertyFlags properties)
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

int main(int argc, char **argv)
{
	// Setup the components from the previous projects
	Setup();

	// Create a array of data to store on the GPU
	unsigned int example_input_buffer_data[64];
	for (unsigned int i = 0; i < 64; i++)
		example_input_buffer_data[i] = i; // Init the array with some basic data


	// Next we need to define how many bytes of data we want to reserve on the GPU
	VkDeviceSize size = sizeof(unsigned int) * 64;
	// Define that we can access the memory throughthe CPU addressable memory space. Using this we can directly set data onto the GPU
	VkMemoryPropertyFlags buffer_memory_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
	// Create a storage variable for the buffer and buffer memory
	VkBuffer buffer = VK_NULL_HANDLE;
	VkDeviceMemory buffer_memory = VK_NULL_HANDLE;



	VkBufferCreateInfo buffer_info = {};
	buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;            // Define the create info type
	buffer_info.size = size;                                             // How big do we want to buffer to be
	buffer_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;              // What type of buffer do we want. Buffers can have multiple types, for example, uniform & vertex buffer.
	                                                                     // for now we want to keep the buffer spetilised to one type as this will allow vulkan to optimize the data.
	buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;                 // There are two modes, exclusive and concurrent. Defines if it can concurrently be used by multiple queue
	                                                                     // families at the same time


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
		buffer_memory_requirements.memoryTypeBits,                      // What memory type vulkan has told us we need
		buffer_memory_properties                                        // What properties do we rquire of our memory
	);


	VkMemoryAllocateInfo memory_allocate_info = {};
	memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;// Allocate info type of the 
	memory_allocate_info.allocationSize = size;                         // How much memory we wish to allocate on the GPU
	memory_allocate_info.memoryTypeIndex = memory_type;                 // What tyoe if memory we want to allocate

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

	// Raw pointer that will point to GPU memory
	void* mapped_memory = nullptr;


	// The last step to get access to our memory is to map it. Mapping the memory gets us a raw pointer to the memory on the gpu
	// that then we can access.
	VkResult mapped_memory_result = vkMapMemory(
		device,                                                         // The device that the memory is on
		buffer_memory,                                                  // The device memory instance
		0,                                                              // Offset from the memorys start that we are accessing
		size,                                                           // How much memory are we accessing
		0,                                                              // Flags (we dont need this for basic buffers)
		&mapped_memory                                                  // The return for the memory pointer
	);

	// Could we map the GPU memory to our CPU accessable pointer
	assert(mapped_memory_result == VK_SUCCESS);

	////////////////////////////////////////
	///// Finished Creating the Buffer ///// 
	////////////////////////////////////////



	// Now we have created the buffers, we can test moving data to it and then retreaving it


	// First we copy the example data to the GPU
	memcpy(
		mapped_memory,                                                  // The destination for our memory (GPU)
		example_input_buffer_data,                                      // Source for the memory (CPU-Ram)
		size                                                            // How much data we are transfering
	);


	// Next to make sure our data got to the GPU fine, we will unmap the data and remap it, pull it and check to see if its all valid

	// Now we unmap the data
	vkUnmapMemory(
		device,
		buffer_memory
	);
	// Null out the memory 
	mapped_memory = nullptr;


	// The last step to get access to our memory is to map it. Mapping the memory gets us a raw pointer to the memory on the gpu
	// that then we can access.
	mapped_memory_result = vkMapMemory(
		device,                                                         // The device that the memory is on
		buffer_memory,                                                  // The device memory instance
		0,                                                              // Offset from the memorys start that we are accessing
		size,                                                           // How much memory are we accessing
		0,                                                              // Flags (we dont need this for basic buffers)
		&mapped_memory                                                  // The return for the memory pointer
	);

	// Could we map the GPU memory to our CPU accessable pointer
	assert(mapped_memory_result == VK_SUCCESS);

	// New array to put the GPU test data into
	unsigned int example_output_buffer_data[64];

	// Move the data back from the GPU to the ram
	memcpy(
		example_output_buffer_data,                                     // The destination for our memory (CPU-Ram)
		mapped_memory,                                                  // The source for our memory (GPU)
		size                                                            // How much data we are transfering
	);

	// Loop through and do a comparson just to make sure the input data is the same as the output
	for (unsigned int i = 0; i < 64; i++)
	{
		assert(example_input_buffer_data[i] == example_output_buffer_data[i]);
	}
	// If we made it hear, then sucsess! we moved data to the gpu without issue and returned it


	////////////////////////////////////
	///// Finished The Buffer Test ///// 
	////////////////////////////////////

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


	// Finish previous projects cleanups
	Destroy();

	return 0;
}