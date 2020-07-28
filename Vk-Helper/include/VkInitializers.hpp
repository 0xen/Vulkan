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

	VkDescriptorSetAllocateInfo DescriptorSetAllocateInfo(VkDescriptorPool descriptor_pool, const VkDescriptorSetLayout& set_layouts, uint32_t descriptor_set_count);

	VkCommandBufferAllocateInfo CommandBufferAllocateInfo(VkCommandPool pool, uint32_t command_buffer_count);

	VkCommandBufferBeginInfo CommandBufferBeginInfo(VkCommandBufferUsageFlags flag);

	VkSubmitInfo SubmitInfo(VkCommandBuffer& buffer);

	VkSubmitInfo SubmitInfo(unsigned int wait_semaphore_count, VkSemaphore* wait_semaphore, unsigned int signal_semaphore_count, 
		VkSemaphore* signal_semaphore, VkPipelineStageFlags& wait_stages,unsigned int command_buffer_count = 1);

	VkImageMemoryBarrier ImageMemoryBarrier();

	VkImageMemoryBarrier ImageMemoryBarrier(VkImage & image, VkFormat & format, VkImageLayout & old_layout, VkImageLayout & new_layout);

	VkRenderPassBeginInfo RenderPassBeginInfo(VkRenderPass render_pass, VkOffset2D offset, VkExtent2D swapchain_extent);

	VkViewport Viewport(float width, float height, float x, float y, float min_depth, float max_depth);

	VkRect2D Scissor(float width, float height, float offset_x, float offset_y);

	VkSamplerCreateInfo SamplerCreateInfo();

	VkImageViewCreateInfo ImageViewCreate(VkImage image, VkFormat format, VkImageAspectFlags aspect_flags);

	VkPresentInfoKHR PresentInfoKHR(unsigned int wait_semaphore_count, VkSemaphore* wait_semaphore,VkSwapchainKHR& swapchain);

	VkDescriptorImageInfo DescriptorImageInfo(VkSampler sampler, VkImageView view, VkImageLayout layout);

	VkDescriptorBufferInfo DescriptorBufferInfo(VkBuffer buffer, uint32_t size, VkDeviceSize offset);

	VkWriteDescriptorSet WriteDescriptorSet(VkDescriptorSet& descriptor_set, VkDescriptorImageInfo& image_info, unsigned int binding);

	VkWriteDescriptorSet WriteDescriptorSet(VkDescriptorSet& descriptor_set, VkDescriptorBufferInfo& buffer_info, unsigned int binding);








}