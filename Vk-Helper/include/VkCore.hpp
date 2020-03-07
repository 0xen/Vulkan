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
}