#include "../logger.h"

#include <vulkan/vulkan.h>
#include <cassert>
#include <vector>

#define ASSERT_VULKAN(val) if(val != VK_SUCCESS) { assert(false); }
#ifndef VK
#define VK(f) (f)
#endif
#ifndef VKA
#define VKA(f) ASSERT_VULKAN(VK(f))
#endif

#define ARRAY_COUNT(array) (sizeof(array) / sizeof((array)[0]))

typedef struct VulkanQueue {
	VkQueue queue;
	uint32_t familyIndex;
} VulkanQueue;

typedef struct VulkanSwapchain {
	VkSwapchainKHR swapchain;
	uint32_t width;
	uint32_t height;
	VkFormat format;
	std::vector<VkImage> images;
	std::vector<VkImageView> imageViews;
} VulkanSwapchain;

typedef struct VulkanPipeline {
	VkPipeline pipeline;
	VkPipelineLayout pipelineLayout;
} VulkanPipeline;

typedef struct VulkanContext {
	VkInstance instance;
	VkPhysicalDevice physicalDevice;
	VkPhysicalDeviceProperties physicalDeviceProperties;
	VkDevice device;
	VulkanQueue graphicsQueue;
} VulkanContext;

typedef struct VulkanBuffer {
	VkBuffer buffer;
	VkDeviceMemory memory;
} VulkanBuffer;

// Init Vulkan (and all the other stuff too like the Swapchain)
VulkanContext* initVulkan(
	uint32_t instanceExtensionCount,
	const char** instanceExtensions,
	uint32_t deviceExtensionCount,
	const char** deviceExtensions
);

// The functions that create stuff ig
VulkanSwapchain createSwapchain(VulkanContext* context, VkSurfaceKHR surface, VkImageUsageFlags usage, VulkanSwapchain* oldSwapchain = 0);
VkRenderPass createRenderPass(VulkanContext* context, VkFormat format);
VkShaderModule createShaderModule(VulkanContext* context, const char* shaderFilename);
VulkanPipeline createPipeline(
	VulkanContext* context,
	const char* vertexShaderFilename,
	const char* fragmentShaderFilename,
	VkRenderPass renderPass,
	uint32_t width,
	uint32_t height,
	VkVertexInputAttributeDescription* attributes,
	uint32_t numAttributes,
	VkVertexInputBindingDescription* binding
);
void createBuffer(
	VulkanContext* context,
	VulkanBuffer* buffer,
	uint64_t size,
	VkBufferUsageFlags usage,
	VkMemoryPropertyFlags memoryProperties
);

// Exit Vulkan (and destroy all the other stuff too like the Swapchain)
void destroySwapchain(VulkanContext* context, VulkanSwapchain* swapchain);
void destroyRenderpass(VulkanContext* context, VkRenderPass renderPass);
void destroyPipeline(VulkanContext* context, VulkanPipeline* pipeline);
void destroyBuffer(VulkanContext* context, VulkanBuffer* buffer);
void exitVulkan(VulkanContext* context);