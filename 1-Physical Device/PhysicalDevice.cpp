#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <memory>

#include <vulkan/vulkan.h>
#include <VkInitializers.hpp>
#include <VkCore.hpp>

using namespace VkHelper;

VkInstance instance;
VkDebugReportCallbackEXT debugger;

// Everything within the Setup is from previous tuturials
// Setup
// - Instance
// - Debugger
void Setup()
{
	// Define what Layers and Extentions we require
	const char *instance_extensions[] = { "VK_EXT_debug_report" };
	const char *instance_layers[] = { "VK_LAYER_LUNARG_standard_validation" };
	// Create the Vulkan Instance
	instance = CreateInstance(
		instance_extensions, 1,
		instance_layers, 1,
		"1 - Physical Device", VK_MAKE_VERSION(1, 0, 0),
		"Vulkan", VK_MAKE_VERSION(1, 0, 0),
		VK_MAKE_VERSION(1, 1, 108));

	// Attach a debugger to the application to give us validation feedback.
	// This is usefull as it tells us about any issues without application
	debugger = CreateDebugger(instance);
}

// Everything within the Destroy is from previous tuturials
// Destroy
// - Debugger
// - Instance
void Destroy()
{
	// Destroy the debug callback
	// We cant directly call vkDestroyDebugReportCallbackEXT as we need to find the pointer within the Vulkan DLL, See function inplmentation for details.
	DestroyDebugger(
		instance, 
		debugger
	);

	// Clean up the vulkan instance
	vkDestroyInstance(
		instance,
		NULL
	);
}

int main(int argc, char **argv)
{
	Setup();





	Destroy();

	return 0;
}