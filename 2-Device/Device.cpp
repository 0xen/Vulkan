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


// Define what Device Extentions we require
const uint32_t physical_device_extention_count = 1;
// Note that this extention list is diffrent from the instance on as we are telling the system what device settings we need.
const char *physical_device_extensions[physical_device_extention_count] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };


// Everything within the Setup is from previous tuturials
// Setup
// - Instance
// - Debugger
// - Physical Device
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

	assert(foundPhysicalDevice);

	
}

// Everything within the Destroy is from previous tuturials
// Destroy
// - Debugger
// - Instance
void Destroy()
{
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

	// Define hoq many queues we will need in our project, for now, we will just create a single queue
	static const float queue_priority = 1.0f;
	VkDeviceQueueCreateInfo queue_create_info = {};                                
	queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;          // What type of creation strucutre is this
	queue_create_info.queueFamilyIndex = physical_devices_queue_family;            // Queue family index we are wanting to make
	queue_create_info.queueCount = 1;                                              // How many queues we are trying to make
	queue_create_info.pQueuePriorities = &queue_priority;                          // Out of all the queues we are creating, how are we prioritizing them?


	// Create the create info for the device itself
	VkDeviceCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;                      // What type of creation strucutre is this
	create_info.pQueueCreateInfos = &queue_create_info;                            // A pointer to the queue create array
	create_info.queueCreateInfoCount = 1;                                          // How many queue create instances are there
	create_info.pEnabledFeatures = &physical_device_features;                      // What features do we want enabled on the device
	create_info.enabledExtensionCount = physical_device_extention_count;           // How many features are we requesting
	create_info.ppEnabledExtensionNames = physical_device_extensions;              // What extentions do we want to enable


	VkDevice device = VK_NULL_HANDLE;
	 // Finish off by creating the device itself
	VkResult result = vkCreateDevice(
		physical_device,
		&create_info,
		nullptr,
		&device
	);

	// Was the vulkan device created sucsessfully
	assert(result == VK_SUCCESS);

	////////////////////////////////////////
	///// Finished Creating the Device ///// 
	////////////////////////////////////////


	// Clean up the device now that the project is stopping
	vkDestroyDevice(
		device,
		nullptr
	);
	// Finish previous projects cleanups
	Destroy();

	return 0;
}