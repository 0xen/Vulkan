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

	VkBufferCreateInfo BufferCreateInfo(VkDeviceSize size, VkBufferUsageFlags usage, VkSharingMode sharing_mode);

	VkMemoryAllocateInfo MemroyAllocateInfo(VkDeviceSize size, uint32_t memory_type);

	VkDescriptorPoolSize DescriptorPoolSize(VkDescriptorType type, uint32_t descriptorCount);

	VkDescriptorPoolCreateInfo DescriptorPoolCreateInfo(const VkDescriptorPoolSize* sizes, uint32_t size_count, uint32_t max_sets);

	VkDescriptorSetLayoutBinding DescriptorSetLayoutBinding(uint32_t binding, VkDescriptorType type, uint32_t descriptor_count, VkShaderStageFlags stage_flags);

	VkDescriptorSetLayoutCreateInfo DescriptorSetLayoutCreateInfo(const VkDescriptorSetLayoutBinding* bindings, uint32_t binding_count);

	VkDescriptorSetAllocateInfo DescriptorSetAllocateInfo(VkDescriptorPool descriptor_pool, const VkDescriptorSetLayout* set_layouts, uint32_t descriptor_set_count);

	VkCommandBufferAllocateInfo CommandBufferAllocateInfo(VkCommandPool pool, uint32_t command_buffer_count);

	VkCommandBufferBeginInfo CommandBufferBeginInfo(VkCommandBufferUsageFlags flag);

	VkSubmitInfo SubmitInfo(VkCommandBuffer & buffer);

	VkImageMemoryBarrier ImageMemoryBarrier();

	VkImageMemoryBarrier ImageMemoryBarrier(VkImage & image, VkFormat & format, VkImageLayout & old_layout, VkImageLayout & new_layout);
}