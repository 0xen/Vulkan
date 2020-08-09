#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <memory>

#include <vulkan/vulkan.h>

VkInstance instance;

// Compare the required layers to the avaliable layers on the system
bool CheckLayersSupport(const char** layers, int count)
{
	// Find out how many layers are avaliable on the system
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	// Using the count, tell the system how many layer definitions we want to read
	// These layer properties are the layers that are avaliable on the system
	std::unique_ptr<VkLayerProperties[]> layerProperties(new VkLayerProperties[layerCount]());
	vkEnumerateInstanceLayerProperties(&layerCount, layerProperties.get());

	// Loop through for each layer we want to check
	for (int i = 0 ; i < count; ++i)
	{
		bool layerFound = false;
		// Loop through for each avaliable system layer and atempt to find our required layer
		for (int j = 0 ; j < layerCount; ++j)
		{
			// Check to see if the layer matches
			if (strcmp(layers[i], layerProperties[j].layerName) == 0)
			{
				layerFound = true;
				break;
			}
		}
		// If we are missing the required layer, report back
		if (!layerFound)
		{
			return false;
		}
	}
	// Found all the layers
	return true;
}


int main(int argc, char **argv)
{

	const char *instance_layers[] = { "VK_LAYER_LUNARG_standard_validation" };
	const char *instance_extensions[] = { "VK_EXT_debug_report" };

	// Check to see if we have the layer requirments
	assert(CheckLayersSupport(instance_layers, 1) && "Unsupported Layers Found");

	VkApplicationInfo app_info = {};
	app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	app_info.pApplicationName = "Instance";						 // Application name
	app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);	     // Application version
	app_info.pEngineName = "Vulkan";                             // Engine name
	app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);           // Engine version
	app_info.apiVersion = VK_MAKE_VERSION(1, 1, 108);            // Required API version
	


	VkInstanceCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	create_info.pApplicationInfo = &app_info;                    // Pointer to the application information created
	create_info.enabledExtensionCount = 1;                       // The amount of extensions we wish to enable
	create_info.ppEnabledExtensionNames = instance_extensions;   // The raw data of the extensions to enable
	create_info.enabledLayerCount = 1;                           // The amount of layers we wish to enable
	create_info.ppEnabledLayerNames = instance_layers;           // The raw data of the layers to enable


	VkResult result = vkCreateInstance(
		&create_info,                                            // Information to pass to the function
		NULL,                                                    // Memory allocation callback
		&instance                                                // The Vulkan instance to be initialized
	);

	// Was the vulkan instance created sucsessfully
	assert(result == VK_SUCCESS);




	////////////////////////////////////////
	///// Finished setting up Instance ///// 
	////////////////////////////////////////




	// Clean up the vulkan instance
	vkDestroyInstance(
		instance,
		NULL
	);

	return 0;
}