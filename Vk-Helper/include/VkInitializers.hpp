#pragma once

#include <vulkan/vulkan.h>

namespace VkHelper
{
	VkApplicationInfo ApplicationInfo(const char* app_name, uint32_t app_ver, const char* engine_name, uint32_t engine_ver, uint32_t api_version);

	VkInstanceCreateInfo InstanceCreateInfo(VkApplicationInfo& app_info, const char** extensions, unsigned int extensions_count, const char** layers, unsigned int layerCount);

	VkDeviceQueueCreateInfo DeviceQueueCreateInfo(const float* queue_priority, uint32_t queue_count, uint32_t queue_family);

	VkDeviceCreateInfo DeviceCreateInfo(VkDeviceQueueCreateInfo* queue_infos, uint32_t queue_count, VkPhysicalDeviceFeatures& physical_device_features,
		const char** extensions, unsigned int extensions_count);

	VkCommandPoolCreateInfo CommandPoolCreateInfo(const uint32_t& queue_family, VkCommandPoolCreateFlags flags);
}