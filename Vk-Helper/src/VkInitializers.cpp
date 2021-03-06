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

VkDescriptorPoolSize VkHelper::DescriptorPoolSize(VkDescriptorType type, uint32_t descriptorCount)
{
	VkDescriptorPoolSize info = {};
	info.type = type;
	info.descriptorCount = descriptorCount;
	return info;
}

VkDescriptorPoolCreateInfo VkHelper::DescriptorPoolCreateInfo(const VkDescriptorPoolSize * sizes, uint32_t size_count, uint32_t max_sets)
{
	VkDescriptorPoolCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	info.poolSizeCount = size_count;
	info.pPoolSizes = sizes;
	info.maxSets = max_sets;
	return info;
}

VkDescriptorSetLayoutBinding VkHelper::DescriptorSetLayoutBinding(uint32_t binding, VkDescriptorType type, uint32_t descriptor_count, VkShaderStageFlags stage_flags)
{
	VkDescriptorSetLayoutBinding info = {};
	info.binding = binding;
	info.descriptorType = type;
	info.descriptorCount = descriptor_count;
	info.stageFlags = stage_flags;
	return info;
}

VkDescriptorSetLayoutCreateInfo VkHelper::DescriptorSetLayoutCreateInfo(const VkDescriptorSetLayoutBinding * bindings, uint32_t binding_count)
{
	VkDescriptorSetLayoutCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	info.bindingCount = binding_count;
	info.pBindings = bindings;
	return info;
}

VkDescriptorSetAllocateInfo VkHelper::DescriptorSetAllocateInfo(VkDescriptorPool descriptor_pool, const VkDescriptorSetLayout& set_layouts, uint32_t descriptor_set_count)
{
	VkDescriptorSetAllocateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	info.descriptorPool = descriptor_pool;
	info.descriptorSetCount = descriptor_set_count;
	info.pSetLayouts = &set_layouts;
	return info;
}

VkCommandBufferAllocateInfo VkHelper::CommandBufferAllocateInfo(VkCommandPool pool, uint32_t command_buffer_count)
{
	VkCommandBufferAllocateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	info.commandPool = pool;
	info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	info.commandBufferCount = command_buffer_count;
	return info;
}

VkCommandBufferBeginInfo VkHelper::CommandBufferBeginInfo(VkCommandBufferUsageFlags flag)
{
	VkCommandBufferBeginInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	info.flags = flag;
	info.pInheritanceInfo = nullptr;
	return info;
}

VkSubmitInfo VkHelper::SubmitInfo(VkCommandBuffer & buffer)
{
	VkSubmitInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	info.commandBufferCount = 1;
	info.pCommandBuffers = &buffer;
	return info;
}

VkSubmitInfo VkHelper::SubmitInfo(unsigned int wait_semaphore_count, VkSemaphore* wait_semaphore, unsigned int signal_semaphore_count,
	VkSemaphore* signal_semaphore, VkPipelineStageFlags& wait_stages, unsigned int command_buffer_count)
{
	VkSubmitInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	info.waitSemaphoreCount = wait_semaphore_count;
	info.pWaitSemaphores = wait_semaphore;
	info.pWaitDstStageMask = &wait_stages;
	info.commandBufferCount = command_buffer_count;
	info.signalSemaphoreCount = signal_semaphore_count;
	info.pSignalSemaphores = signal_semaphore;
	return info;
}

VkImageMemoryBarrier VkHelper::ImageMemoryBarrier()
{
	VkImageMemoryBarrier image_memory_barrier{};
	image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	image_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	image_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	return image_memory_barrier;
}

bool HasStencilComponent(VkFormat format)
{
	return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

VkImageMemoryBarrier VkHelper::ImageMemoryBarrier(VkImage & image, VkFormat & format, VkImageLayout & old_layout, VkImageLayout & new_layout)
{
	VkImageMemoryBarrier barrier = ImageMemoryBarrier();
	barrier.oldLayout = old_layout;
	barrier.newLayout = new_layout;
	barrier.image = image;
	if (new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
	{
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

		if (format != VK_FORMAT_UNDEFINED && HasStencilComponent(format))
		{
			barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}
	}
	else
	{
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	if (old_layout == VK_IMAGE_LAYOUT_PREINITIALIZED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	}
	else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	}
	else if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	}
	else if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	}
	else if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_GENERAL)
	{
		barrier.srcAccessMask = 0;
	}
	return barrier;
}

VkRenderPassBeginInfo VkHelper::RenderPassBeginInfo(VkRenderPass render_pass, VkOffset2D offset, VkExtent2D swapchain_extent)
{
	VkRenderPassBeginInfo render_pass_info = {};
	render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	render_pass_info.renderPass = render_pass;
	render_pass_info.renderArea.offset = offset;
	render_pass_info.renderArea.extent = swapchain_extent;
	render_pass_info.clearValueCount = 0;
	render_pass_info.pClearValues = nullptr;
	return render_pass_info;
}

VkViewport VkHelper::Viewport(float width, float height, float x, float y, float min_depth, float max_depth)
{
	VkViewport viewport = {};
	viewport.x = x;
	viewport.y = y;
	viewport.width = width;
	viewport.height = height;
	viewport.minDepth = min_depth;
	viewport.maxDepth = max_depth;
	return viewport;
}

VkRect2D VkHelper::Scissor(float width, float height, float offset_x, float offset_y)
{
	VkRect2D scissor{};
	scissor.extent.width = width;
	scissor.extent.height = height;
	scissor.offset.x = offset_x;
	scissor.offset.y = offset_y;
	return scissor;
}

VkSamplerCreateInfo VkHelper::SamplerCreateInfo()
{
	VkSamplerCreateInfo sampler_create_info{};
	sampler_create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	sampler_create_info.maxAnisotropy = 1.0f;

	// Point
	sampler_create_info.magFilter = VK_FILTER_NEAREST;
	sampler_create_info.minFilter = VK_FILTER_NEAREST;
	// Byliniar
	//sampler_info.magFilter = VK_FILTER_LINEAR;
	//sampler_info.minFilter = VK_FILTER_LINEAR;
	sampler_create_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	sampler_create_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler_create_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler_create_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler_create_info.mipLodBias = 0.0f;
	sampler_create_info.compareOp = VK_COMPARE_OP_NEVER;
	sampler_create_info.minLod = 0.0f;
	// Set max level-of-detail to mip level count of the texture
	sampler_create_info.maxLod = (float)1;

	// Do not use anisotropy
	sampler_create_info.maxAnisotropy = 1.0;
	sampler_create_info.anisotropyEnable = VK_FALSE;

	sampler_create_info.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

	return sampler_create_info;
}

VkImageViewCreateInfo VkHelper::ImageViewCreate(VkImage image, VkFormat format, VkImageAspectFlags aspect_flags)
{

	VkImageViewCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	create_info.image = image;
	create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
	create_info.format = format;
	create_info.components.r = VK_COMPONENT_SWIZZLE_R;
	create_info.components.g = VK_COMPONENT_SWIZZLE_G;
	create_info.components.b = VK_COMPONENT_SWIZZLE_B;
	create_info.components.a = VK_COMPONENT_SWIZZLE_A;
	create_info.subresourceRange.aspectMask = aspect_flags;// VK_IMAGE_ASPECT_COLOR_BIT;
	create_info.subresourceRange.baseMipLevel = 0;
	create_info.subresourceRange.levelCount = 1;
	create_info.subresourceRange.baseArrayLayer = 0;
	create_info.subresourceRange.layerCount = 1;
	return create_info;

}

VkPresentInfoKHR VkHelper::PresentInfoKHR(unsigned int wait_semaphore_count, VkSemaphore* wait_semaphore, VkSwapchainKHR& swapchain)
{
	VkPresentInfoKHR info = {};
	info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	info.waitSemaphoreCount = wait_semaphore_count;
	info.pWaitSemaphores = wait_semaphore;
	info.swapchainCount = 1;
	// The swapchain will be recreated whenever the window is resized or the KHR becomes invalid
	// But the pointer to our swapchain will remain intact
	info.pSwapchains = &swapchain;
	info.pResults = nullptr;
	return info;
}

VkDescriptorImageInfo VkHelper::DescriptorImageInfo(VkSampler sampler, VkImageView view, VkImageLayout layout)
{
	VkDescriptorImageInfo descriptorImageInfo = {};
	// Provide the descriptor info the texture data it requires
	descriptorImageInfo.sampler = sampler;
	descriptorImageInfo.imageView = view;
	descriptorImageInfo.imageLayout = layout;
	return descriptorImageInfo;
}

VkDescriptorBufferInfo VkHelper::DescriptorBufferInfo(VkBuffer buffer, uint32_t size, VkDeviceSize offset)
{
	VkDescriptorBufferInfo buffer_info = {};
	buffer_info.buffer = buffer;
	buffer_info.offset = offset;
	buffer_info.range = size;
	return buffer_info;
}

VkWriteDescriptorSet VkHelper::WriteDescriptorSet(VkDescriptorSet& descriptor_set, VkDescriptorImageInfo& image_info, unsigned int binding)
{
	VkWriteDescriptorSet descriptorWrite = {};
	descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrite.dstSet = descriptor_set;
	descriptorWrite.dstBinding = binding;
	descriptorWrite.dstArrayElement = 0;
	descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrite.descriptorCount = 1;											// How many descriptors we are updating
	descriptorWrite.pBufferInfo = VK_NULL_HANDLE;
	descriptorWrite.pImageInfo = VK_NULL_HANDLE;
	descriptorWrite.pTexelBufferView = VK_NULL_HANDLE;
	descriptorWrite.pNext = VK_NULL_HANDLE;

	static const int offset = offsetof(VkWriteDescriptorSet, pImageInfo);
	// Transfer the VkWriteDescriptorSet into the VkDescriptorImageInfo
	VkDescriptorImageInfo** data = reinterpret_cast<VkDescriptorImageInfo**>(reinterpret_cast<uint8_t*>(&descriptorWrite) + offset);
	*data = &image_info;
	return descriptorWrite;
}

VkWriteDescriptorSet VkHelper::WriteDescriptorSet(VkDescriptorSet& descriptor_set, VkDescriptorImageInfo* image_info, unsigned int image_info_count, unsigned int binding)
{
	VkWriteDescriptorSet descriptorWrite = {};
	descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrite.dstSet = descriptor_set;
	descriptorWrite.dstBinding = binding;
	descriptorWrite.dstArrayElement = 0;
	descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrite.descriptorCount = image_info_count;											// How many descriptors we are updating
	descriptorWrite.pBufferInfo = VK_NULL_HANDLE;
	descriptorWrite.pImageInfo = VK_NULL_HANDLE;
	descriptorWrite.pTexelBufferView = VK_NULL_HANDLE;
	descriptorWrite.pNext = VK_NULL_HANDLE;

	static const int offset = offsetof(VkWriteDescriptorSet, pImageInfo);
	// Transfer the VkWriteDescriptorSet into the VkDescriptorImageInfo
	VkDescriptorImageInfo** data = reinterpret_cast<VkDescriptorImageInfo**>(reinterpret_cast<uint8_t*>(&descriptorWrite) + offset);
	*data = image_info;
	return descriptorWrite;
}

VkWriteDescriptorSet VkHelper::WriteDescriptorSet(VkDescriptorSet& descriptor_set, VkDescriptorType type, VkDescriptorImageInfo& image_info, unsigned int binding)
{
	VkWriteDescriptorSet descriptor_write = {};
	descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptor_write.dstSet = descriptor_set; // write to this descriptor set.
	descriptor_write.dstBinding = binding; // write to the first, and only binding.
	descriptor_write.dstArrayElement = 0;
	descriptor_write.descriptorType = type; // Type of buffer
	descriptor_write.descriptorCount = 1; // update a single descriptor.
	descriptor_write.pImageInfo = &image_info;
	return descriptor_write;
}

VkWriteDescriptorSet VkHelper::WriteDescriptorSet(VkDescriptorSet& descriptor_set, VkDescriptorType type, VkDescriptorBufferInfo& buffer_info, unsigned int binding)
{
	VkWriteDescriptorSet descriptorWrite = {};
	descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrite.dstSet = descriptor_set;
	descriptorWrite.dstBinding = binding;
	descriptorWrite.dstArrayElement = 0;
	descriptorWrite.descriptorType = type;
	descriptorWrite.descriptorCount = 1;											// How many descriptors we are updating
	descriptorWrite.pBufferInfo = &buffer_info;
	descriptorWrite.pImageInfo = VK_NULL_HANDLE;
	descriptorWrite.pTexelBufferView = VK_NULL_HANDLE;
	descriptorWrite.pNext = VK_NULL_HANDLE;

	return descriptorWrite;
}

VkWriteDescriptorSet VkHelper::WriteDescriptorSet(VkDescriptorSet& descriptor_set, VkDescriptorType type, VkWriteDescriptorSetAccelerationStructureNV& buffer_info, unsigned int binding)
{
	VkWriteDescriptorSet descriptorWrite = {};
	descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrite.dstSet = descriptor_set;
	descriptorWrite.dstBinding = binding;
	descriptorWrite.dstArrayElement = 0;
	descriptorWrite.descriptorType = type;
	descriptorWrite.descriptorCount = 1;
	descriptorWrite.pBufferInfo = VK_NULL_HANDLE;
	descriptorWrite.pImageInfo = VK_NULL_HANDLE;
	descriptorWrite.pTexelBufferView = VK_NULL_HANDLE;
	descriptorWrite.pNext = VK_NULL_HANDLE;

	static const int offset = offsetof(VkWriteDescriptorSet, pNext);

	VkWriteDescriptorSetAccelerationStructureNV** data = reinterpret_cast<VkWriteDescriptorSetAccelerationStructureNV**>(reinterpret_cast<uint8_t*>(&descriptorWrite) + offset);

	*data = &buffer_info;

	return descriptorWrite;
}

VkRayTracingShaderGroupCreateInfoNV VkHelper::RayTracingShaderGroupCreateNV(VkRayTracingShaderGroupTypeNV type)
{
	VkRayTracingShaderGroupCreateInfoNV groupInfo{};
	groupInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_NV;
	groupInfo.pNext = nullptr;
	groupInfo.type = type;
	groupInfo.generalShader = VK_SHADER_UNUSED_NV;
	groupInfo.closestHitShader = VK_SHADER_UNUSED_NV;
	groupInfo.anyHitShader = VK_SHADER_UNUSED_NV;
	groupInfo.intersectionShader = VK_SHADER_UNUSED_NV;
	return groupInfo;
}

VkPipelineShaderStageCreateInfo VkHelper::PipelineShaderStageCreateInfo(VkShaderModule& shader, const char* main, VkShaderStageFlagBits flag)
{
	VkPipelineShaderStageCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	info.stage = flag;
	info.module = shader;
	info.pName = main;
	return info;
}

VkSpecializationInfo VkHelper::SpecializationInfo(uint32_t mapEntryCount, const VkSpecializationMapEntry* mapEntries, size_t dataSize, const void* data)
{
	VkSpecializationInfo specializationInfo{};
	specializationInfo.mapEntryCount = mapEntryCount;
	specializationInfo.pMapEntries = mapEntries;
	specializationInfo.dataSize = dataSize;
	specializationInfo.pData = data;
	return specializationInfo;
}

VkAccelerationStructureInfoNV VkHelper::AccelerationStructureInfoNV(VkAccelerationStructureTypeNV type, VkBuildAccelerationStructureFlagsNV flags, const VkGeometryNV* prt, uint32_t count, uint32_t instance_count)
{
	VkAccelerationStructureInfoNV accelerationStructureInfo{};
	accelerationStructureInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
	accelerationStructureInfo.type = type;
	accelerationStructureInfo.flags = flags;
	accelerationStructureInfo.instanceCount = instance_count;  // The bottom-level AS can only contain explicit geometry, and no instances
	accelerationStructureInfo.geometryCount = count;
	accelerationStructureInfo.pGeometries = prt;

	return accelerationStructureInfo;
}

VkAccelerationStructureCreateInfoNV VkHelper::AccelerationStructureCreateInfoNV(VkAccelerationStructureInfoNV structure_info)
{
	VkAccelerationStructureCreateInfoNV info{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_NV };
	info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_NV;
	info.pNext = nullptr;
	info.info = structure_info;
	info.compactedSize = 0;
	return info;
}

VkAccelerationStructureMemoryRequirementsInfoNV VkHelper::AccelerationStructureMemoryRequirmentsInfoNV(VkAccelerationStructureNV str)
{
	VkAccelerationStructureMemoryRequirementsInfoNV memoryRequirementsInfo;
	memoryRequirementsInfo.sType =
		VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_NV;
	memoryRequirementsInfo.pNext = nullptr;
	memoryRequirementsInfo.accelerationStructure = str;
	memoryRequirementsInfo.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_OBJECT_NV;
	return memoryRequirementsInfo;
}

VkWriteDescriptorSetAccelerationStructureNV VkHelper::WriteDescriptorSetAccelerator(VkAccelerationStructureNV& acceleration)
{
	VkWriteDescriptorSetAccelerationStructureNV descriptorAccelerationStructureInfo;
	descriptorAccelerationStructureInfo.sType =
		VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_NV;
	descriptorAccelerationStructureInfo.pNext = nullptr;
	descriptorAccelerationStructureInfo.accelerationStructureCount = 1;
	descriptorAccelerationStructureInfo.pAccelerationStructures = &acceleration;
	return descriptorAccelerationStructureInfo;
}

VkBindAccelerationStructureMemoryInfoNV VkHelper::AccelerationStructureMemoryInfoNV(VkAccelerationStructureNV str, VkDeviceMemory memory)
{
	VkBindAccelerationStructureMemoryInfoNV info{};
	info.sType = VK_STRUCTURE_TYPE_BIND_ACCELERATION_STRUCTURE_MEMORY_INFO_NV;
	info.pNext = nullptr;
	info.accelerationStructure = str;
	info.memory = memory;
	info.memoryOffset = 0;
	info.deviceIndexCount = 0;
	info.pDeviceIndices = nullptr;
	return info;
}

VkAccelerationStructureInfoNV VkHelper::AccelerationStructureInfo(VkBuildAccelerationStructureFlagsNV flags, unsigned int instanceCount)
{
	VkAccelerationStructureInfoNV info;
	info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
	info.pNext = nullptr;
	info.flags = flags;
	info.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_NV;
	info.geometryCount = 0;
	info.pGeometries = nullptr;
	info.instanceCount = instanceCount;
	return info;
}

VkAccelerationStructureInfoNV VkHelper::AccelerationStructureInfo(VkBuildAccelerationStructureFlagsNV flags, VkGeometryNV& buffer)
{
	VkAccelerationStructureInfoNV info;
	info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
	info.pNext = nullptr;
	info.flags = flags;
	info.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_NV;
	info.geometryCount = 1;
	info.pGeometries = &buffer;
	info.instanceCount = 0;
	return info;
}

VkGeometryNV VkHelper::CreateRayTraceGeometry(VkBuffer vertexBuffer, VkDeviceSize vertexOffsetInBytes, uint32_t vertexCount, VkDeviceSize vertexSizeInBytes, VkBuffer indexBuffer, VkDeviceSize indexOffsetInBytes, uint32_t indexCount, VkBuffer transformBuffer, VkDeviceSize transformOffsetInBytes, bool isOpaque)
{
	VkGeometryNV geometry;
	geometry.sType = VK_STRUCTURE_TYPE_GEOMETRY_NV;
	geometry.pNext = nullptr;
	geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_NV;
	geometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_GEOMETRY_TRIANGLES_NV;
	geometry.geometry.triangles.pNext = nullptr;
	geometry.geometry.triangles.vertexData = vertexBuffer;
	geometry.geometry.triangles.vertexOffset = vertexOffsetInBytes;
	geometry.geometry.triangles.vertexCount = vertexCount;
	geometry.geometry.triangles.vertexStride = vertexSizeInBytes;
	// Limitation to 3xfloat32 for vertices
	geometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
	geometry.geometry.triangles.indexData = indexBuffer;
	geometry.geometry.triangles.indexOffset = indexOffsetInBytes;
	geometry.geometry.triangles.indexCount = indexCount;
	// Limitation to 32-bit indices
	geometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
	geometry.geometry.triangles.transformData = transformBuffer;
	geometry.geometry.triangles.transformOffset = transformOffsetInBytes;
	geometry.geometry.aabbs = { VK_STRUCTURE_TYPE_GEOMETRY_AABB_NV };
	geometry.flags = isOpaque ? VK_GEOMETRY_OPAQUE_BIT_NV : 0;

	return geometry;
}

VkPhysicalDeviceRayTracingPropertiesNV VkHelper::CreatePhysicalDeviceRayTracingProperties()
{
	VkPhysicalDeviceRayTracingPropertiesNV device_raytracing_properties = {};
	device_raytracing_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PROPERTIES_NV;
	device_raytracing_properties.pNext = nullptr;
	device_raytracing_properties.maxRecursionDepth = 0;
	device_raytracing_properties.shaderGroupHandleSize = 0;
	return device_raytracing_properties;
}

VkPhysicalDeviceProperties2 VkHelper::CreatePhysicalDeviceProperties2(VkPhysicalDeviceRayTracingPropertiesNV& ray_traceing_properties)
{
	VkPhysicalDeviceProperties2 physical_device_properties2 = {};
	physical_device_properties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
	physical_device_properties2.pNext = &ray_traceing_properties;
	physical_device_properties2.properties = {};
	return physical_device_properties2;
}
