#include "vulkan_base.h"

bool initVulkanInstance(
	VulkanContext* context,
	uint32_t instanceExtensionCount,
	const char** instanceExtensions
)
{
	uint32_t layerPropertyCount;
	VKA(vkEnumerateInstanceLayerProperties(&layerPropertyCount, 0));
	VkLayerProperties* layerProperties = new VkLayerProperties[layerPropertyCount];
	VKA(vkEnumerateInstanceLayerProperties(&layerPropertyCount, layerProperties));
	for (uint32_t i = 0; i < layerPropertyCount; i++) {
#ifdef VULKAN_INFO_OUTPUT
		LOG_INFO(layerProperties[i].layerName);
		LOG_INFO(layerProperties[i].description);
#endif
	}

	const char* enabledLayers[] = {
		"VK_LAYER_KHRONOS_validation",
	};

	/*uint32_t availableInstanceExtensionCount;
	vkEnumerateInstanceExtensionProperties(0, &availableInstanceExtensionCount, 0);
	VkExtensionProperties* availableInstanceExtensionProperties = new VkExtensionProperties[availableInstanceExtensionCount];
	vkEnumerateInstanceExtensionProperties(0, &availableInstanceExtensionCount, availableInstanceExtensionProperties);
	for (uint32_t i = 0; i < availableInstanceExtensionCount; i++) {
		LOG_INFO(availableInstanceExtensionProperties[i].extensionName);
	}
	delete[] availableInstanceExtensionProperties;*/

	VkApplicationInfo applicationInfo = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
	applicationInfo.pApplicationName = "Flowing Rendering Engine";
	applicationInfo.applicationVersion = VK_MAKE_VERSION(0, 0, 1);
	applicationInfo.apiVersion = VK_API_VERSION_1_3;

	VkInstanceCreateInfo createInfo = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
	createInfo.pApplicationInfo = &applicationInfo;
	createInfo.enabledLayerCount = ARRAY_COUNT(enabledLayers);
	createInfo.ppEnabledLayerNames = enabledLayers;
	createInfo.enabledExtensionCount = instanceExtensionCount;
	createInfo.ppEnabledExtensionNames = instanceExtensions;

	vkCreateInstance(&createInfo, 0, &context->instance);

	if (VK(vkCreateInstance(&createInfo, 0, &context->instance)) != VK_SUCCESS) {
		LOG_ERROR("Error creating Vulkan instance");
		return false;
	}

	return true;
}

bool selectPhysicalDevice(VulkanContext* context) {
	uint32_t numDevices = 0;
	VKA(vkEnumeratePhysicalDevices(context->instance, &numDevices, 0));
	if (numDevices == 0) {
		LOG_ERROR("Failed to find Vulkan Device!");
		context->physicalDevice = 0;
		return false;
	}
	VkPhysicalDevice* physicalDevices = new VkPhysicalDevice[numDevices];
	VKA(vkEnumeratePhysicalDevices(context->instance, &numDevices, physicalDevices));
	LOG_INFO("Found ", numDevices, " Physical Device(s):");
	for (uint32_t i = 0; i < numDevices; i++) {
		VkPhysicalDeviceProperties properties = {};
		VK(vkGetPhysicalDeviceProperties(physicalDevices[i], &properties));
		LOG_INFO("Physical Device ", i, ": ", properties.deviceName);
	}
	
	context->physicalDevice = physicalDevices[0];
	VK(vkGetPhysicalDeviceProperties(context->physicalDevice, &context->physicalDeviceProperties));
	LOG_INFO("Selected Physical Device: ", context->physicalDeviceProperties.deviceName);

	delete[] physicalDevices;

	return true;
}

bool createLogicalDevice(
	VulkanContext* context,
	uint32_t deviceExtensionCount,
	const char** deviceExtensions
)
{
	// Queues
	uint32_t numQueueFamilies = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(context->physicalDevice, &numQueueFamilies, 0);
	VkQueueFamilyProperties* queueFamilies = new VkQueueFamilyProperties[numQueueFamilies];
	vkGetPhysicalDeviceQueueFamilyProperties(context->physicalDevice, &numQueueFamilies, queueFamilies);

	uint32_t graphicsQueueIndex = 0;
	for (uint32_t i = 0; i < numQueueFamilies; i++) {
		VkQueueFamilyProperties queueFamily = queueFamilies[i];
		if (queueFamily.queueCount > 0) {
			if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
				graphicsQueueIndex = i;
				break;
			}
		}
	}
	
	float priorities[] = { 1.0f };
	VkDeviceQueueCreateInfo queueCreateInfo = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
	queueCreateInfo.queueFamilyIndex = graphicsQueueIndex;
	queueCreateInfo.queueCount = 1;
	queueCreateInfo.pQueuePriorities = priorities;

	VkPhysicalDeviceFeatures enabledFeatures = {};

	VkDeviceCreateInfo createInfo = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
	createInfo.queueCreateInfoCount = 1;
	createInfo.pQueueCreateInfos = &queueCreateInfo;
	createInfo.enabledExtensionCount = deviceExtensionCount;
	createInfo.ppEnabledExtensionNames = deviceExtensions;
	createInfo.pEnabledFeatures = &enabledFeatures;

	VkResult res = vkCreateDevice(context->physicalDevice, &createInfo, 0, &context->device);

	if (res) {
		LOG_ERROR("Failed to create Vulkan logical Device");
		return false;
	}

	// Acquire queues
	context->graphicsQueue.familyIndex = graphicsQueueIndex;
	VK(vkGetDeviceQueue(context->device, graphicsQueueIndex, 0, &context->graphicsQueue.queue));

	return true;
}

VulkanContext* initVulkan(
	uint32_t instanceExtensionCount, 
	const char** instanceExtensions, 
	uint32_t deviceExtensionCount, 
	const char** deviceExtensions
)
{
	VulkanContext* context = new VulkanContext;

	if (!initVulkanInstance(context, instanceExtensionCount, instanceExtensions))
		return 0;

	if (!selectPhysicalDevice(context))
		return 0;

	if (!createLogicalDevice(context, deviceExtensionCount, deviceExtensions))
		return 0;

	return context;
}

void exitVulkan(VulkanContext* context) {
	VKA(vkDeviceWaitIdle(context->device));
	VK(vkDestroyDevice(context->device, 0));

	vkDestroyInstance(context->instance, 0);
}