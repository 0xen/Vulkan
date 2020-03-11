#include <VkInitializers.hpp>
#include "..\include\VkInitializers.hpp"

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

VkDeviceQueueCreateInfo VkHelper::DeviceQueueCreateInfo(const float * queue_priority, uint32_t queue_count, uint32_t queue_family)
{
	VkDeviceQueueCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;    // What type of creation strucutre is this
	info.queueFamilyIndex = queue_family;                       // Queue family index we are wanting to make
	info.queueCount = queue_count;                              // How many queues we are trying to make
	info.pQueuePriorities = queue_priority;                     // Out of all the queues we are creating, how are we prioritizing them?
	return info;
}

VkDeviceCreateInfo VkHelper::DeviceCreateInfo(VkDeviceQueueCreateInfo * queue_infos, uint32_t queue_count, VkPhysicalDeviceFeatures & physical_device_features, const char ** extensions, unsigned int extensions_count)
{
	// Create the create info for the device itself
	VkDeviceCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;          // What type of creation strucutre is this
	info.pQueueCreateInfos = queue_infos;                       // A pointer to the queue create array
	info.queueCreateInfoCount = queue_count;                    // How many queue create instances are there
	info.pEnabledFeatures = &physical_device_features;          // What features do we want enabled on the device
	info.enabledExtensionCount = extensions_count;              // How many features are we requesting
	info.ppEnabledExtensionNames = extensions;                  // What extentions do we want to enable
	return info;
}

VkCommandPoolCreateInfo VkHelper::CommandPoolCreateInfo(const uint32_t & queue_family, VkCommandPoolCreateFlags flags)
{

	VkCommandPoolCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;    // Create instance type we are creating
	info.queueFamilyIndex = queue_family;                       // What queue family we are wanting to use to send commands to the GPU
	info.flags = flags;                                         // Allows any commands we create, the ability to be reset. This is helpfull as we wont need to
															    // keep allocating new commands,we can reuse them
	return info;
}

VkBufferCreateInfo VkHelper::BufferCreateInfo(VkDeviceSize size, VkBufferUsageFlags usage, VkSharingMode sharing_mode)
{
	VkBufferCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;          // Define the create info type
	info.size = size;                                           // How big do we want to buffer to be
	info.usage = usage;                                         // What type of buffer do we want. Buffers can have multiple types, for example, uniform & vertex buffer.
													            // for now we want to keep the buffer spetilised to one type as this will allow vulkan to optimize the data.
	info.sharingMode = sharing_mode;                            // There are two modes, exclusive and concurrent. Defines if it can concurrently be used by multiple queue
														        // families at the same time
	return info;
}

VkMemoryAllocateInfo VkHelper::MemroyAllocateInfo(VkDeviceSize size, uint32_t memory_type)
{
	VkMemoryAllocateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;        // Allocate info type of the 
	info.allocationSize = size;                                 // How much memory we wish to allocate on the GPU
	info.memoryTypeIndex = memory_type;                         // What tyoe if memory we want to allocate
	return info;
}
