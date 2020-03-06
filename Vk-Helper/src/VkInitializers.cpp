#include <VkInitializers.hpp>

VkApplicationInfo VkHelper::ApplicationInfo(const char * app_name, uint32_t app_ver, const char * engine_name, uint32_t engine_ver, uint32_t api_version)
{
	VkApplicationInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	info.pApplicationName = app_name;                           // Application name
	info.applicationVersion = app_ver;	                        // Application version
	info.pEngineName = engine_name;                             // Engine name
	info.engineVersion = engine_ver;                            // Engine version
	info.apiVersion = api_version;                              // Required API version
	return info;
}

VkInstanceCreateInfo VkHelper::InstanceCreateInfo(VkApplicationInfo & app_info, const char ** extensions, unsigned int extensions_count, const char ** layers, unsigned int layerCount)
{
	VkInstanceCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	info.pApplicationInfo = &app_info;                          // Pointer to the application information created
	info.enabledExtensionCount = extensions_count;              // The amount of extensions we wish to enable
	info.ppEnabledExtensionNames = extensions;                  // The raw data of the extensions to enable
	info.enabledLayerCount = layerCount;                        // The amount of layers we wish to enable
	info.ppEnabledLayerNames = layers;                          // The raw data of the layers to enable
	return info;
}
