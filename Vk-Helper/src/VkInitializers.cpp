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

VkDescriptorSetAllocateInfo VkHelper::DescriptorSetAllocateInfo(VkDescriptorPool descriptor_pool, const VkDescriptorSetLayout * set_layouts, uint32_t descriptor_set_count)
{
	VkDescriptorSetAllocateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	info.descriptorPool = descriptor_pool;
	info.descriptorSetCount = descriptor_set_count;
	info.pSetLayouts = set_layouts;
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
