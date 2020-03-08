#pragma once

#include <vulkan/vulkan.h>

namespace VkHelper
{
	// Compare the required layers to the avaliable layers on the system
	bool CheckLayersSupport(const char** layers, int count);

	VkInstance CreateInstance(const char** extensions, unsigned int extensions_count, const char** layers, unsigned int layerCount,
		const char* app_name, uint32_t app_ver, const char* engine_name, uint32_t engine_ver, uint32_t api_version);

	// Attach a debugger to the application to give us validation feedback.
	// This is usefull as it tells us about any issues without application
	VkDebugReportCallbackEXT CreateDebugger(const VkInstance& instance);

	void DestroyDebugger(const VkInstance& instance, const VkDebugReportCallbackEXT& debugger);

	// Check to see if a physical device has the required extention support
	bool HasRequiredExtentions(const VkPhysicalDevice& physical_device, const char** required_extentions, const uint32_t& required_extention_count);

	// Find a queue family that has the required queue types
	bool GetQueueFamily(const VkPhysicalDevice& physical_device, VkQueueFlags required_queue_flags, uint32_t& queue_family_index);

	bool GetPhysicalDevice(const VkInstance& instance, VkPhysicalDevice& physical_device, VkPhysicalDeviceProperties& device_properties, uint32_t& queue_family_index,
		VkPhysicalDeviceFeatures& device_features, VkPhysicalDeviceMemoryProperties& device_mem_properties, const char ** physical_device_extentions, const unsigned int extention_count,
		VkQueueFlags required_queue_flags);

	VkDevice CreateDevice(const VkPhysicalDevice& physical_device, VkDeviceQueueCreateInfo* queue_infos, uint32_t queue_info_count, VkPhysicalDeviceFeatures& physical_device_features,
		const char** extensions, unsigned int extensions_count);
}