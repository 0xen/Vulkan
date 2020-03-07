#include <VkCore.hpp>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <memory>

// A basic debug callback. A more advanced one could be created, but this will do for basic debugging
VKAPI_ATTR VkBool32 VKAPI_CALL MyDebugReportCallback(
	VkDebugReportFlagsEXT       flags,
	VkDebugReportObjectTypeEXT  objectType,
	uint64_t                    object,
	size_t                      location,
	int32_t                     messageCode,
	const char*                 pLayerPrefix,
	const char*                 pMessage,
	void*                       pUserData)
{
	printf(pMessage);
	return VK_FALSE;
}


// Compare the required layers to the avaliable layers on the system
bool VkHelper::CheckLayersSupport(const char** layers, int count)
{
	// Find out how many layers are avaliable on the system
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	// Using the count, tell the system how many layer definitions we want to read
	// These layer properties are the layers that are avaliable on the system
	std::unique_ptr<VkLayerProperties[]> layerProperties(new VkLayerProperties[layerCount]());
	vkEnumerateInstanceLayerProperties(&layerCount, layerProperties.get());

	// Loop through for each layer we want to check
	for (int i = 0; i < count; ++i)
	{
		bool layerFound = false;
		// Loop through for each avaliable system layer and atempt to find our required layer
		for (int j = 0; j < layerCount; ++j)
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

VkInstance VkHelper::CreateInstance(const char ** extensions, unsigned int extensions_count, const char ** layers, unsigned int layerCount, const char * app_name, uint32_t app_ver, const char * engine_name, uint32_t engine_ver, uint32_t api_version)
{
	VkInstance instance;

	VkApplicationInfo app_info = {};
	app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	app_info.pApplicationName = "0 - Instance";                  // Application name
	app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);	     // Application version
	app_info.pEngineName = "Vulkan";                             // Engine name
	app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);           // Engine version
	app_info.apiVersion = VK_MAKE_VERSION(1, 1, 108);            // Required API version



	VkInstanceCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	create_info.pApplicationInfo = &app_info;                    // Pointer to the application information created
	create_info.enabledExtensionCount = extensions_count;        // The amount of extensions we wish to enable
	create_info.ppEnabledExtensionNames = extensions;            // The raw data of the extensions to enable
	create_info.enabledLayerCount = layerCount;                  // The amount of layers we wish to enable
	create_info.ppEnabledLayerNames = layers;                    // The raw data of the layers to enable


	VkResult result = vkCreateInstance(
		&create_info,                                            // Information to pass to the function
		NULL,                                                    // Memory allocation callback
		&instance                                                // The Vulkan instance to be initialized
	);

	// Was the vulkan instance created sucsessfully
	assert(result == VK_SUCCESS);

	return instance;
}

// Attach a debugger to the application to give us validation feedback.
// This is usefull as it tells us about any issues without application
VkDebugReportCallbackEXT VkHelper::CreateDebugger(const VkInstance& instance)
{
	// Get the function pointer for the debug callback function within VK
	PFN_vkCreateDebugReportCallbackEXT vkCreateDebugReportCallbackEXT =
		reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT>
		(vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT"));


	/*PFN_vkDebugReportMessageEXT vkDebugReportMessageEXT =
		reinterpret_cast<PFN_vkDebugReportMessageEXT>
		(vkGetInstanceProcAddr(instance, "vkDebugReportMessageEXT"));
	PFN_vkDestroyDebugReportCallbackEXT vkDestroyDebugReportCallbackEXT =
		reinterpret_cast<PFN_vkDestroyDebugReportCallbackEXT>
		(vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT"));*/


	// Define a CreateInfo for our new callback
	VkDebugReportCallbackCreateInfoEXT callbackCreateInfo;
	callbackCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;            // Callback Type
	callbackCreateInfo.pNext = nullptr;
	callbackCreateInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT |                            // What we wukk be notified about
		VK_DEBUG_REPORT_WARNING_BIT_EXT |                                                 //...
		VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;                                      //...
	callbackCreateInfo.pfnCallback = &MyDebugReportCallback;                              // Our function that will be called on a callback
	callbackCreateInfo.pUserData = nullptr;                                               // A custom data pointer that the user can define. Since we are calling a non member function
	                                                                                      // it may be usefull to pass a pointer instance of the engine or rendering libary, in this case
	                                                                                      // we dont need anything

	VkDebugReportCallbackEXT callback;

	// Create the new callback
	VkResult result = vkCreateDebugReportCallbackEXT(instance, &callbackCreateInfo, nullptr, &callback);

	// Was the vulkan callback created sucsessfully
	assert(result == VK_SUCCESS);

	return callback;
}

void VkHelper::DestroyDebugger(const VkInstance & instance, const VkDebugReportCallbackEXT & debugger)
{
	PFN_vkDestroyDebugReportCallbackEXT vkDestroyDebugReportCallbackEXT =
		reinterpret_cast<PFN_vkDestroyDebugReportCallbackEXT>
		(vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT"));

	// Destroy the debug callback
	vkDestroyDebugReportCallbackEXT(
		instance,
		debugger,
		nullptr
	);
}
