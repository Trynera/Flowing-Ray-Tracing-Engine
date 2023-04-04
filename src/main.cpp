#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_vulkan.h>

#include "utils/Camera.h"

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/transform.hpp>

#include "logger.h"
#include "vkbase/vulkan_base.h"

#define FRAMES_IN_FLIGHT 2

VulkanContext* context;
VkSurfaceKHR surface;
VkRenderPass renderPass;
VulkanSwapchain swapchain;
std::vector<VkFramebuffer> framebuffers;
VulkanPipeline pipeline;
VkCommandPool commandPools[FRAMES_IN_FLIGHT];
VkCommandBuffer commandBuffers[FRAMES_IN_FLIGHT];
VkFence fences[FRAMES_IN_FLIGHT];
VkSemaphore acquireSemaphores[FRAMES_IN_FLIGHT];
VkSemaphore releaseSemaphores[FRAMES_IN_FLIGHT];
VulkanBuffer vertexBuffer;
VulkanBuffer uniformBuffer;

Camera camera;

bool mouseAbsorbed = false;

glm::mat4 rotationMatrix(1);

void handleInput(
	SDL_Window* window,
	SDL_Event* event, 
	float deltaTime, 
	glm::vec3& cameraPos, 
	float& cameraYaw, 
	float& cameraPitch, 
	glm::mat4* rotationMatrix
) {
	bool moved = false;

	int mouseX = { event->motion.x };
	int mouseY = { event->motion.y };

	float xOffset = (float)(mouseX - 1280.0f / 2.0f);
	float yOffset = (float)(mouseY - 720.0f / 2.0f);

	if (xOffset != 0.0f || yOffset != 0.0f) moved = true;

	cameraYaw += xOffset * 0.002f;
	cameraPitch += yOffset * 0.002f;

	if (cameraPitch > 1.5707f)
		cameraPitch = 1.5707f;
	if (cameraPitch < -1.5707f)
		cameraPitch = -1.5707f;

	*rotationMatrix = glm::rotate(glm::rotate(glm::mat4(1), cameraPitch, glm::vec3(1.0f, 0.0f, 0.0f)), cameraYaw, glm::vec3(0.0f, 1.0f, 0.0f));
}

bool handleMessage(SDL_Window* window, int width, int height) {
	SDL_Event event;

	while (SDL_PollEvent(&event)) {
		float deltaTime = 0.0f;
		float lastFrame = 0.0f;

		float currentFrame = (float)(SDL_GetTicks()) / 1000.0f;

		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		glm::vec3 forward = glm::vec3(glm::vec4(0.0f, 0.0f, -1.0f, 0.0f) );

		glm::vec3 up(0.0f, 1.0f, 0.0f);
		glm::vec3 right = glm::cross(forward, up);

		glm::vec3 movementDirection(0);
		float multiplier = 1;

		switch (event.type) {
		case SDL_QUIT: return false;
		}
		if (event.type == SDL_MOUSEBUTTONDOWN) {
			if (event.button.button == SDL_BUTTON_RIGHT) {
				SDL_WarpMouseInWindow(window, width / 2.0f, height / 2.0f);
				SDL_SetRelativeMouseMode(SDL_TRUE);
				mouseAbsorbed = true;
				handleInput(window, &event, deltaTime, camera.pos, camera.cameraYaw, camera.cameraPitch, &rotationMatrix);
			}
		}
		else if (event.type == SDL_MOUSEBUTTONUP) {
			SDL_SetRelativeMouseMode(SDL_FALSE);
			mouseAbsorbed = false;
			rotationMatrix = glm::rotate(glm::rotate(glm::mat4(1), camera.cameraPitch, glm::vec3(1.0f, 0.0f, 0.0f)), camera.cameraYaw, glm::vec3(0.0f, 1.0f, 0.0f));
		}
		else if (event.type == SDL_KEYDOWN) {
			if (event.key.keysym.sym == SDLK_w)
				camera.pos.y = 0.0f;
			else if (SDLK_s)
				movementDirection -= forward;
			if (SDLK_a)
				movementDirection -= right;
			else if (SDLK_d)
				movementDirection += right;
			if (SDLK_SPACE)
				movementDirection += up;
			else if (SDLK_LSHIFT)
				movementDirection -= up;
			if (SDLK_LCTRL)
				multiplier = 10;
			else
				multiplier = 1;

			if (glm::length(movementDirection) > 0.0f)
				camera.pos += glm::normalize(movementDirection) * (float)deltaTime * (float)multiplier;
		}
	}
	return true;
}

// Recreates the Render Pass
void recreateRenderPass() {
	if (renderPass) {
		for (uint32_t i = 0; i < framebuffers.size(); i++)
			VK(vkDestroyFramebuffer(context->device, framebuffers[i], 0));
		destroyRenderpass(context, renderPass);
	}
	framebuffers.clear();

	renderPass = createRenderPass(context, swapchain.format);
	framebuffers.resize(swapchain.images.size());
	for (uint32_t i = 0; i < swapchain.images.size(); i++) {
		VkFramebufferCreateInfo createInfo = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
		createInfo.renderPass = renderPass;
		createInfo.attachmentCount = 1;
		createInfo.pAttachments = &swapchain.imageViews[i];
		createInfo.width = swapchain.width;
		createInfo.height = swapchain.height;
		createInfo.layers = 1;
		VKA(vkCreateFramebuffer(context->device, &createInfo, 0, &framebuffers[i]));
	}
}

// Ignore this, it's not important
float vertexData[] = {
	1.0f, -1.0f,
	camera.pos.x, camera.pos.y, camera.pos.z,

	1.0f, 1.0f,
	camera.pos.x, camera.pos.y, camera.pos.z,

	-1.0f, -1.0f,
	camera.pos.x, camera.pos.y, camera.pos.z,

	1.0f, 1.0f,
	camera.pos.x, camera.pos.y, camera.pos.z,

	-1.0f, -1.0f,
	camera.pos.x, camera.pos.y, camera.pos.z,

	-1.0f, 1.0f,
	camera.pos.x, camera.pos.y, camera.pos.z,
};

void initApp(SDL_Window* window) {
	uint32_t instanceExtensionCount;
	SDL_Vulkan_GetInstanceExtensions(window, &instanceExtensionCount, 0);
	const char** enabledInstanceExtensions = new const char* [instanceExtensionCount];
	SDL_Vulkan_GetInstanceExtensions(window, &instanceExtensionCount, enabledInstanceExtensions);

	const char* enabledDeviceExtensions[]{ VK_KHR_SWAPCHAIN_EXTENSION_NAME };
	context = initVulkan(
		instanceExtensionCount,
		enabledInstanceExtensions,
		ARRAY_COUNT(enabledDeviceExtensions),
		enabledDeviceExtensions
	);
	SDL_Vulkan_CreateSurface(window, context->instance, &surface);
	swapchain = createSwapchain(context, surface, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, 0);

	recreateRenderPass();

	VkVertexInputAttributeDescription vertexAttributeDescriptions[2] = {};
	vertexAttributeDescriptions[0].binding = 0;
	vertexAttributeDescriptions[0].location = 0;
	vertexAttributeDescriptions[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	vertexAttributeDescriptions[0].offset = 0;
	vertexAttributeDescriptions[1].binding = 0;
	vertexAttributeDescriptions[1].location = 1;
	vertexAttributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	vertexAttributeDescriptions[1].offset = sizeof(float) * 2;
	VkVertexInputBindingDescription vertexInputBinding = {};
	vertexInputBinding.binding = 0;
	vertexInputBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	vertexInputBinding.stride = sizeof(float) * 5;

	{ // Creates the Render Pipeline
		pipeline = createPipeline(context, "../shaders/vert.spv", "../shaders/frag.spv", renderPass, swapchain.width, swapchain.height, vertexAttributeDescriptions, ARRAY_COUNT(vertexAttributeDescriptions), &vertexInputBinding);
	}

	for (uint32_t i = 0; i < ARRAY_COUNT(fences); i++) { // Sets Create Info for Fence
		VkFenceCreateInfo createInfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
		createInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
		VKA(vkCreateFence(context->device, &createInfo, 0, &fences[i]));
	}

	// Make sure that the acquireSemaphore and the releaseSemaphore are the same length!
	for (uint32_t i = 0; i < ARRAY_COUNT(acquireSemaphores); i++) { // Sets Semaphore
		VkSemaphoreCreateInfo createInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
		VKA(vkCreateSemaphore(context->device, &createInfo, 0, &acquireSemaphores[i]));
		VKA(vkCreateSemaphore(context->device, &createInfo, 0, &releaseSemaphores[i]));
	}

	for (uint32_t i = 0; i < ARRAY_COUNT(commandPools); i++) { // Sets Command Pool
		VkCommandPoolCreateInfo createInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
		createInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
		createInfo.queueFamilyIndex = context->graphicsQueue.familyIndex;
		VKA(vkCreateCommandPool(context->device, &createInfo, 0, &commandPools[i]));
	}
	for (uint32_t i = 0; i < ARRAY_COUNT(commandPools); i++) { // Sets Command Buffer
		VkCommandBufferAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
		allocateInfo.commandPool = commandPools[i];
		allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocateInfo.commandBufferCount = 1;
		VKA(vkAllocateCommandBuffers(context->device, &allocateInfo, &commandBuffers[i]));
	}

	createBuffer(
		context, 
		&vertexBuffer, 
		sizeof(vertexData), 
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
	);
	void* data;
	VKA(vkMapMemory(context->device, vertexBuffer.memory, 0, sizeof(vertexData), 0, &data));
	memcpy(data, vertexData, sizeof(vertexData));
	VK(vkUnmapMemory(context->device, vertexBuffer.memory));
}

// Recreates the Swapchain (used for resizing windows)
void recreateSwapchain() {
	VulkanSwapchain oldSwapchain = swapchain;

	VkSurfaceCapabilitiesKHR surfaceCapabilities;
	VKA(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(context->physicalDevice, surface, &surfaceCapabilities));
	if (surfaceCapabilities.currentExtent.width == 0 || surfaceCapabilities.currentExtent.height == 0) {
		return;
	}


	VKA(vkDeviceWaitIdle(context->device));
	swapchain = createSwapchain(context, surface, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, &oldSwapchain);

	destroySwapchain(context, &oldSwapchain);
	recreateRenderPass();
}

void renderApp() {
	uint32_t imageIndex = 0;
	static uint32_t frameIndex = 0;

	// Wait for the n-2 frame to finish to be able to reuse its acquireSemaphore in vkAcquireNextImageKHR
	VKA(vkWaitForFences(context->device, 1, &fences[frameIndex], VK_TRUE, UINT64_MAX));

	VkResult result = VK(vkAcquireNextImageKHR(context->device, swapchain.swapchain, UINT64_MAX, acquireSemaphores[frameIndex], 0, &imageIndex));
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
		// Swapchain is out of date
		recreateSwapchain();
		return;
	}
	VKA(vkResetFences(context->device, 1, &fences[frameIndex]));
	ASSERT_VULKAN(result);

	VKA(vkResetCommandPool(context->device, commandPools[frameIndex], 0));

	void* data;
	VKA(vkMapMemory(context->device, vertexBuffer.memory, 0, sizeof(vertexData), 0, &data));
	memcpy(data, vertexData, sizeof(vertexData));
	VK(vkUnmapMemory(context->device, vertexBuffer.memory));

	VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	{
		VkCommandBuffer commandBuffer = commandBuffers[frameIndex];
		VKA(vkBeginCommandBuffer(commandBuffer, &beginInfo));

		VkViewport viewport = { 0.0f, 0.0f, (float)swapchain.width, (float)swapchain.height };
		VkRect2D scissor = { {0, 0}, {swapchain.width, swapchain.height} };
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

		VkClearValue clearValue = { 0.0f, 1.0f, 1.0f, 1.0f };
		VkRenderPassBeginInfo beginInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
		beginInfo.renderPass = renderPass;
		beginInfo.framebuffer = framebuffers[imageIndex];
		beginInfo.renderArea = { {0, 0}, {swapchain.width, swapchain.height} };
		beginInfo.clearValueCount = 1;
		beginInfo.pClearValues = &clearValue;
		vkCmdBeginRenderPass(commandBuffer, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipeline);

		VkDeviceSize offset = 0;
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer.buffer, &offset);

		vkCmdDraw(commandBuffer, 6, 1, 0, 0);

		vkCmdEndRenderPass(commandBuffer);

		VKA(vkEndCommandBuffer(commandBuffer));
	}

	VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffers[frameIndex];
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &acquireSemaphores[frameIndex];
	VkPipelineStageFlags waitMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	submitInfo.pWaitDstStageMask = &waitMask;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &releaseSemaphores[frameIndex];
	VKA(vkQueueSubmit(context->graphicsQueue.queue, 1, &submitInfo, fences[frameIndex]));

	VkPresentInfoKHR presentInfo = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &swapchain.swapchain;
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &releaseSemaphores[frameIndex];
	result = VK(vkQueuePresentKHR(context->graphicsQueue.queue, &presentInfo));
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
		// Swapchain is out of date
		recreateSwapchain();
	}
	else {
		ASSERT_VULKAN(result);
	}

	frameIndex = (frameIndex + 1) % FRAMES_IN_FLIGHT;
}

void shutdownApp(SDL_Window* window) {
	VKA(vkDeviceWaitIdle(context->device));

	destroyBuffer(context, &vertexBuffer);

	for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
		VK(vkDestroyFence(context->device, fences[i], 0));
		VK(vkDestroySemaphore(context->device, acquireSemaphores[i], 0));
		VK(vkDestroySemaphore(context->device, releaseSemaphores[i], 0));
	}
	for (uint32_t i = 0; i < ARRAY_COUNT(commandPools); i++)
		VK(vkDestroyCommandPool(context->device, commandPools[i], 0));

	destroyPipeline(context, &pipeline);

	for (uint32_t i = 0; i < framebuffers.size(); i++)
		VK(vkDestroyFramebuffer(context->device, framebuffers[i], 0));
	framebuffers.clear();
	destroyRenderpass(context, renderPass);
	destroySwapchain(context, &swapchain);
	VK(vkDestroySurfaceKHR(context->instance, surface, 0));
	exitVulkan(context);

	SDL_DestroyWindow(window);
	SDL_Quit();
}

int main() {
	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		LOG_ERROR("Error initializing SDL: ", SDL_GetError());
		return 1;
	}

	int window_width = 1280;
	int window_height = 720;
	SDL_Window* window = SDL_CreateWindow(
		"Vulkan Real-Time Ray Tracer", 
		SDL_WINDOWPOS_CENTERED, 
		SDL_WINDOWPOS_CENTERED, 
		window_width, 
		window_height, 
		SDL_WINDOW_VULKAN
	);
	if (!window) {
		LOG_ERROR("Error creating SDL window: ", SDL_GetError());
		return 1;
	}

	initApp(window);

	while (handleMessage(window, window_width, window_height)) {
		vertexData[2] = camera.pos.x;
		vertexData[3] = camera.pos.y;
		vertexData[4] = camera.pos.z;
		vertexData[7] = camera.pos.x;
		vertexData[8] = camera.pos.y;
		vertexData[9] = camera.pos.z;
		vertexData[12] = camera.pos.x;
		vertexData[13] = camera.pos.y;
		vertexData[14] = camera.pos.z;
		vertexData[17] = camera.pos.x;
		vertexData[18] = camera.pos.y;
		vertexData[19] = camera.pos.z;
		vertexData[22] = camera.pos.x;
		vertexData[23] = camera.pos.y;
		vertexData[24] = camera.pos.z;
		renderApp();
	}

	shutdownApp(window);

	return 0;
}