#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <memory>

#include <vulkan/vulkan.h>
#include <VkInitializers.hpp>
#include <VkCore.hpp>

VkInstance instance;
VkDebugReportCallbackEXT debugger;



// Everything within the Setup is from previous tuturials
// Setup
// - Instance
// - Debugger
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
		"1 - Physical Device", VK_MAKE_VERSION(1, 0, 0),
		"Vulkan", VK_MAKE_VERSION(1, 0, 0),
		VK_MAKE_VERSION(1, 1, 108));

	// Attach a debugger to the application to give us validation feedback.
	// This is usefull as it tells us about any issues without application
	debugger = VkHelper::CreateDebugger(instance);
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

// Check to see if a physical device has the required extention support
bool HasRequiredExtentions(const VkPhysicalDevice& physical_device, const char** required_extentions, const uint32_t& required_extention_count)
{
	// Get all the extentions on device
	// First we need to get the extention count
	uint32_t extension_count;
	vkEnumerateDeviceExtensionProperties(
		physical_device,                                       // Physical Device we are checking
		nullptr,                                               // 
		&extension_count,                                      // A return parameter to allow us to get the amount of extentions on the gpu
		nullptr                                                // A return pointer that sets a pointer array full of the extention names
		                                                       // When set to nullptr, we are telling the system we want to get the device count
	);

	// Now get all the avaliable extentions on the device
	std::unique_ptr<VkExtensionProperties[]> available_extensions(new VkExtensionProperties[extension_count]());
	vkEnumerateDeviceExtensionProperties(
		physical_device,                                       // Physical Device we are checking
		nullptr,                                               // 
		&extension_count,                                      // A return parameter to allow us to get the amount of extentions on the gpu
		available_extensions.get()                             // A return pointer that sets a pointer array full of the extention names
	);

	// Loop through all wanted extentions to make sure they all exist
	for (int i = 0; i < required_extention_count; ++i)
	{
		bool extention_found = false;
		// Loop through for each avaliable device extention and atempt to find our required extention
		for (int j = 0; j < extension_count; ++j)
		{
			// Check to see if the layer matches
			if (strcmp(required_extentions[i], available_extensions[j].extensionName) == 0)
			{
				extention_found = true;
				break;
			}
		}
		// If we are missing the extention layer, report back
		if (!extention_found)
		{
			return false;
		}
	}
	return true;
}

// Find a queue family that has the required queue types
bool GetQueueFamily(const VkPhysicalDevice& physical_device, VkQueueFlags required_queue_flags, uint32_t& queue_family_index)
{
	// Fist we need to get the amount of queue families on the system
	uint32_t queue_family_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(
		physical_device,                                       // Physical Device we are checking
		&queue_family_count,                                   // A return parameter to allow us to get the amount of queue families on the gpu
		nullptr                                                // A return pointer that sets a pointer array full of the extention names
	);

	// Next get the queue families
	std::unique_ptr<VkQueueFamilyProperties[]> queue_families(new VkQueueFamilyProperties[queue_family_count]());
	vkGetPhysicalDeviceQueueFamilyProperties(
		physical_device,                                       // Physical Device we are checking
		&queue_family_count,                                   // A return parameter to allow us to get the amount of queue families on the gpu
		queue_families.get()                                   // A return pointer that sets a pointer array full of the extention names
		                                                       // When set to nullptr, we are telling the system we want to get the queue family count
	);

	// Loop through all the queue families to find one that matches our criteria
	for (int i = 0; i < queue_family_count; ++i)
	{
		// Make sure there are queues in the queue family
		if (queue_families[i].queueCount > 0)
		{
			// Check if the queue family has the correct queues
			// Using bitwise AND check that the queue flags are set
			if ((queue_families[i].queueFlags & required_queue_flags) == required_queue_flags)
			{

				// Later on in tuturial 6-Swapchain, we introduce the consept of a surface that we render too.
				// The surface is the display and we want to make sure that the queue family supports rendering to
				// the surface. A inplemented example can be seen in VkCore.cpp but here is the validation code needed later
				// when we have added surfaces

				/*VkBool32 present_support = false;
				vkGetPhysicalDeviceSurfaceSupportKHR(
					physical_device,
					i,
					surface,
					&present_support
				);
				if (present_support)
				{
					queue_family_index = i;
					return true;
				}*/
				queue_family_index = i;
				return true;
			}
		}
	}
	// Return a invalid state
	return false;
}

int main(int argc, char **argv)
{
	Setup();


	// Find all avaliable devices within the system
	// Get the device count
	uint32_t device_count = 0;
	vkEnumeratePhysicalDevices(
		instance,                                           // Refrence to our Vulkan Instance
		&device_count,                                      // A return parameter for the amount of system devices
		nullptr                                             // A pointer return parameter for all devices, if left nullptr, then we are saying, we just want the device count
	);

	// Get the Device Instances
	std::unique_ptr<VkPhysicalDevice[]> devices(new VkPhysicalDevice[device_count]());
	// Get the physical devices
	vkEnumeratePhysicalDevices(
		instance,                                           // Refrence to our Vulkan Instance
		&device_count,                                      // A return parameter for the amount of system devices
		devices.get()                                       // A pointer return parameter to get all devices
	);

	// Define what Device Extentions we require
	const uint32_t extention_count = 1;
	// In this case we define that we will need 'VK_KHR_SWAPCHAIN_EXTENSION_NAME' This will be used in future tuturials but defines that
	// We will require the mechanism that allows for images to be displayed on the display known as a swapchain.

	// Note that this extention list is diffrent from the instance on as we are telling the system what device settings we need.
	const char *device_extensions[extention_count] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

	// this will store the device we have chosen
	VkPhysicalDevice chosen_physical_device = VK_NULL_HANDLE;
	uint32_t chosen_physical_devices_queue_family = 0;
	VkPhysicalDeviceProperties chosen_physical_device_properties;
	VkPhysicalDeviceFeatures chosen_physical_device_features;
	VkPhysicalDeviceMemoryProperties chosen_physical_device_mem_properties;


	// Now we have all the physical devices that are inside the device, we need to find a suitable one for our needs
	for (int i = 0 ; i < device_count; i++)
	{
		// First we need to check that the device has all the required extentions
		if (HasRequiredExtentions(devices[i], device_extensions, extention_count))
		{
			// Next each physical device has a list of queue families that controll the flow of commands to and from the CPU side of the system
			// to the GPU side. Each queue family contains a diffrent amount of Queues, some contain all of them, some may contain a mixed bag or
			// some may only contain 1.

			// The common queues we consern ourself with is the GraphicsQueue, but there is also compute (for general compute tasks), 
			// transfer (moving data between the devices) and sparse (for device memory managment).

			uint32_t queue_family = 0;
			if (GetQueueFamily(devices[i], VkQueueFlagBits::VK_QUEUE_GRAPHICS_BIT, queue_family))
			{
				// Now we know the device is valid, there may be several valid devices within the system and we need a way of choosing between them
				// So we can get the properties of the device and find a way of prioritising them
				VkPhysicalDeviceProperties physical_device_properties;
				vkGetPhysicalDeviceProperties(
					devices[i],
					&physical_device_properties
				);
				// Find the features that are avaliable on the device
				VkPhysicalDeviceFeatures physical_device_features;
				vkGetPhysicalDeviceFeatures(
					devices[i],
					&physical_device_features
				);

				// Get all information about the devices memory
				VkPhysicalDeviceMemoryProperties physical_device_mem_properties;
				vkGetPhysicalDeviceMemoryProperties(
					devices[i],
					&physical_device_mem_properties
				);


				// Some further checks could be done in the future if you require a serten properties or features to be on the device
				// ...


				// First if the chosen device is null and the only GPU that reports back is a intergrated gpu then use it untill we find a deticated one
				if (chosen_physical_device == VK_NULL_HANDLE || chosen_physical_device != VK_NULL_HANDLE && physical_device_properties.deviceType == VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
				{
					chosen_physical_device = devices[i];
					chosen_physical_devices_queue_family = queue_family;
					chosen_physical_device_properties = physical_device_properties;
					chosen_physical_device_features = physical_device_features;
					chosen_physical_device_mem_properties = physical_device_mem_properties;
				}
			}
		}
	}

	// Check to see if the device is valid
	assert(chosen_physical_device != VK_NULL_HANDLE);






	///////////////////////////////////////////////
	///// Finished Selecting Physical Device ///// 
	///////////////////////////////////////////////



	// We do not need to clean up the physical device
	Destroy();

	return 0;
}