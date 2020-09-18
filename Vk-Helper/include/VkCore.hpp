#pragma once

#include <vulkan/vulkan.h>
#include <memory>

namespace VkHelper
{
	struct FrameBufferAttachment
	{
		VkImage image;
		VkDeviceMemory memory;
		VkImageView view;
		VkFormat format;
		VkSampler sampler;
	};

	struct VulkanAttachments
	{
		FrameBufferAttachment color, depth;
	};


	struct ModelInstance
	{
		unsigned int vertex_offset = 0;
		unsigned int index_offset = 0;
		unsigned int vertex_count = 0;
		unsigned int index_count = 0;

		VkGeometryNV geometry;

		VkAccelerationStructureNV acceleration_structure = VK_NULL_HANDLE;


		VkBuffer scratch_buffer = VK_NULL_HANDLE;
		VkDeviceMemory scratch_buffer_memory = VK_NULL_HANDLE;
		// Raw pointer that will point to GPU memory
		void* scratch_mapped_buffer_memory = nullptr;

		VkBuffer result_buffer = VK_NULL_HANDLE;
		VkDeviceMemory result_buffer_memory = VK_NULL_HANDLE;
		// Raw pointer that will point to GPU memory
		void* result_mapped_buffer_memory = nullptr;
	};

	struct VkGeometryInstance
	{
		/// Transform matrix, containing only the top 3 rows
		float transform[12];
		/// Instance index
		uint32_t instanceId : 24;
		/// Visibility mask
		uint32_t mask : 8;
		/// Index of the hit group which will be invoked when a ray hits the instance
		uint32_t instanceOffset : 24;
		/// Instance flags, such as culling
		uint32_t flags : 8;
		/// Opaque handle of the bottom-level acceleration structure
		uint64_t accelerationStructureHandle;
	};

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
	bool GetQueueFamily(const VkPhysicalDevice& physical_device, VkQueueFlags required_queue_flags, uint32_t& queue_family_index, VkSurfaceKHR surface = VK_NULL_HANDLE);

	bool GetPhysicalDevice(const VkInstance& instance, VkPhysicalDevice& physical_device, VkPhysicalDeviceProperties& device_properties, uint32_t& queue_family_index,
		VkPhysicalDeviceFeatures& device_features, VkPhysicalDeviceMemoryProperties& device_mem_properties, const char ** physical_device_extentions, const unsigned int extention_count,
		VkQueueFlags required_queue_flags, VkSurfaceKHR surface = VK_NULL_HANDLE);

	VkDevice CreateDevice(const VkPhysicalDevice& physical_device, VkDeviceQueueCreateInfo* queue_infos, uint32_t queue_info_count, VkPhysicalDeviceFeatures& physical_device_features,
		const char** extensions, unsigned int extensions_count);

	VkCommandPool CreateCommandPool(const VkDevice& device, const uint32_t& queue_family, VkCommandPoolCreateFlags flags);

	uint32_t FindMemoryType(const VkPhysicalDeviceMemoryProperties& physical_device_mem_properties, uint32_t type_filter, VkMemoryPropertyFlags properties);

	bool CreateBuffer(const VkDevice& device, const VkPhysicalDeviceMemoryProperties& physical_device_mem_properties, VkBuffer& buffer, VkDeviceMemory& buffer_memory,
		VkDeviceSize size, VkBufferUsageFlags usage, VkSharingMode sharing_mode, VkMemoryPropertyFlags buffer_memory_properties);

	VkDescriptorPool CreateDescriptorPool(const VkDevice& device, VkDescriptorPoolSize* pool_sizes, uint32_t pool_sizes_count, uint32_t max_sets);

	VkDescriptorSetLayout CreateDescriptorSetLayout(const VkDevice& device, VkDescriptorSetLayoutBinding* layout_bindings, uint32_t layout_binding_count);

	VkDescriptorSet AllocateDescriptorSet(const VkDevice& device, const VkDescriptorPool& descriptor_pool, const VkDescriptorSetLayout& descriptor_set_layout, uint32_t count);

	VkSwapchainKHR CreateSwapchain(const VkPhysicalDevice & physical_device, const VkDevice & device, const VkSurfaceKHR& surface, VkSurfaceCapabilitiesKHR & surface_capabilities,
		VkSurfaceFormatKHR& surface_format, VkPresentModeKHR& present_mode, uint32_t window_width, uint32_t window_height, 
		uint32_t& swapchain_image_count, std::unique_ptr<VkImage>& swapchain_images, std::unique_ptr<VkImageView>& swapchain_image_views);

	void CreateImage(const VkDevice& device, const VkPhysicalDeviceMemoryProperties& physical_device_mem_properties, uint32_t width, uint32_t height, VkFormat format, 
		VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage & image, VkDeviceMemory & image_memory, VkImageLayout initialLayout);

	VkCommandBuffer BeginSingleTimeCommands(const VkDevice& device, const VkCommandPool& command_pool);

	void EndSingleTimeCommands(const VkDevice& device, const VkQueue& queue, VkCommandBuffer command_buffer, VkCommandPool command_pool);

	void TransitionImageLayout(VkCommandBuffer command_buffer, VkImage image, VkFormat format, VkImageLayout old_layout, VkImageLayout new_layout, VkImageSubresourceRange subresourceRange);

	void TransitionImageLayout(const VkDevice& device, const VkQueue& queue, VkCommandPool command_pool, VkImage image, VkFormat format, VkImageLayout old_layout, VkImageLayout new_layout, VkImageSubresourceRange subresourceRange);

	void CreateAttachmentImages(const VkDevice& device, const VkQueue& queue, uint32_t width, uint32_t height, const VkPhysicalDeviceMemoryProperties& physical_device_mem_properties, const VkPhysicalDeviceFeatures& physical_device_features,
		const VkPhysicalDeviceProperties& physical_device_properties, const VkCommandPool& command_pool, VkFormat format, VkImageUsageFlags usage, VkHelper::FrameBufferAttachment & attachment);

	VkFormat FindSupportedFormat(const VkPhysicalDevice& physical_device, const VkFormat* candidate_formats, const uint32_t candidate_format_count, VkImageTiling tiling, VkFormatFeatureFlags features);
	
	VkRenderPass CreateRenderPass(const VkPhysicalDevice & physical_device, const VkDevice& device, VkFormat present_format, VkFormat image_color_format, const uint32_t& swapchain_image_count, 
		const VkPhysicalDeviceMemoryProperties& physical_device_mem_properties, const VkPhysicalDeviceFeatures& physical_device_features, const VkPhysicalDeviceProperties& physical_device_properties, 
		const VkCommandPool& command_pool, const VkQueue& queue, uint32_t width, uint32_t height, std::unique_ptr<VkFramebuffer>& framebuffers, std::unique_ptr<VulkanAttachments>& framebuffer_attachments,
		std::unique_ptr<VkImageView>& swapchain_image_views);

	void ReadShaderFile(const char* filename, char*& data, unsigned int& size);

	VkPipeline CreateGraphicsPipeline(const VkPhysicalDevice & physical_device, const VkDevice& device, const VkRenderPass& renderpass, VkPipelineLayout& graphics_pipeline_layout,
		uint32_t shader_count, const char* shader_path[], VkShaderStageFlagBits* shader_stages_bits, VkShaderModule* shader_modules,
		uint32_t descriptor_set_layout_count, const VkDescriptorSetLayout* descriptor_set_layout,
		uint32_t vertex_input_attribute_description_count, const VkVertexInputAttributeDescription* vertex_input_attribute_descriptions,
		uint32_t vertex_input_binding_description_count, const VkVertexInputBindingDescription* vertex_input_binding_descriptions,
		uint32_t dynamic_state_count = 0, VkDynamicState* dynamic_states = nullptr,
		VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VkPolygonMode polygon_mode = VK_POLYGON_MODE_FILL, float line_width = 1.0f,
		VkCullModeFlags cull_mode = VK_CULL_MODE_FRONT_BIT, VkBool32 depth_write_enable = VK_TRUE, VkBool32 depth_test_enable = VK_TRUE);

	VkPipeline CreateComputePipeline(const VkDevice& device, VkPipelineLayout& compute_pipeline_layout,
		const char* shader_path, VkShaderModule& shader_module, uint32_t descriptor_set_layout_count, const VkDescriptorSetLayout* descriptor_set_layout);

	VkShaderModule LoadShader(const VkDevice& device, const char* path);

	void CreateImageSampler(const VkDevice& device, const VkImage& image, VkFormat format, VkImageView& imageView, VkSampler& sampler);

	void SetImageLayout(VkCommandBuffer cmdbuffer, VkImage image, VkImageLayout oldImageLayout,
		VkImageLayout newImageLayout, VkImageSubresourceRange subresourceRange);

	void CreateFence(const VkDevice& device, std::unique_ptr<VkFence>& fences, unsigned int count);

	void CreateVkSemaphore(const VkDevice& device, VkSemaphore& semaphore);

	void GetPhysicalDevicePropertiesAndFeatures2(const VkPhysicalDevice& physical_device, VkPhysicalDeviceRayTracingPropertiesNV& device_raytracing_properties, VkPhysicalDeviceProperties2& physical_device_properties2, VkPhysicalDeviceFeatures2& device_features2);

	void CreateBottomLevelASBuffer(VkDevice device, const VkPhysicalDeviceMemoryProperties& physical_device_mem_properties, VkCommandBuffer commandBuffer, ModelInstance& model);

	void BuildAccelerationStructure(VkDevice device, VkCommandBuffer commandBuffer, VkAccelerationStructureInfoNV& acceleration_structure_info, VkBuffer instance_buffer,
		VkAccelerationStructureNV& acceleration_structure, VkBuffer& scratch_buffer);


	VkAccelerationStructureNV CreateTopLevelASBuffer(VkDevice device, const VkPhysicalDeviceMemoryProperties& physical_device_mem_properties,
		VkCommandBuffer commandBuffer, unsigned int model_instances_count,
		VkBuffer& as_scratch_buffer, VkDeviceMemory& as_scratch_buffer_memory, VkDeviceSize& as_scratch_size,
		VkBuffer& as_result_buffer, VkDeviceMemory& as_result_buffer_memory, VkDeviceSize& as_result_size,
		VkBuffer& as_instance_buffer, VkDeviceMemory& as_instance_buffer_memory, VkDeviceSize& as_instances_size);



}