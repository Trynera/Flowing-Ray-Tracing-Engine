#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_vulkan.h>

#include "utils/Camera.h"
#include "utils/GpuModel.h"

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/transform.hpp>

#include "logger.h"
#include "vkbase/vulkan_base.h"
#include "utils/RtScene.h"
#include <algorithm>

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

VkDescriptorSetLayout descriptorSetLayout;
VkDescriptorPool uniformDescriptorPool;
VkDescriptorSet uniformDescriptorSets[FRAMES_IN_FLIGHT];
VulkanBuffer uniformBuffers[FRAMES_IN_FLIGHT];

VkDescriptorPool listDescriptorPool;
VulkanBuffer listBuffers[FRAMES_IN_FLIGHT];

std::vector<Triangle> triangles;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

float currentFrame = (float)(SDL_GetTicks()) / 1000.0f;
float firstFrame = currentFrame;

Camera camera;

bool mouseAbsorbed = false;

glm::mat4 rotationMatrix(1);

bool fullscreen;
int window_width = 1280;
int window_height = 720;

struct UniformBufferObject {
	alignas(16) glm::vec3 camPos;
	alignas(16) glm::mat4 rotationMatrix;
	alignas(16) glm::ivec2 screenRes;
	alignas(4) float time;
};

HitList world;

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

	float xOffset = (float)(mouseX - (float)window_width / 2.0f);
	float yOffset = (float)(mouseY - (float)window_height / 2.0f);

	if (xOffset != 0.0f || yOffset != 0.0f) moved = true;

	cameraYaw += xOffset * 0.002f;
	cameraPitch += yOffset * 0.002f;

	if (cameraPitch > 1.5707f)
		cameraPitch = 1.5707f;
	if (cameraPitch < -1.5707f)
		cameraPitch = -1.5707f;

	*rotationMatrix = glm::rotate(glm::rotate(glm::mat4(1), cameraPitch, glm::vec3(1.0f, 0.0f, 0.0f)), cameraYaw, glm::vec3(0.0f, 1.0f, 0.0f));
}

void recreateRenderPass();
void recreateSwapchain();

bool handleMessage(SDL_Window* window, int width, int height) {
	SDL_Event event;

	glm::vec3 forward = glm::vec3(glm::vec4(0.0f, 0.0f, -1.0f, 0.0f));

	glm::vec3 up(0.0f, 1.0f, 0.0f);
	glm::vec3 right = glm::cross(forward, up);

	glm::vec3 movementDirection(0);
	float multiplier = 1;

	const uint8_t* state = SDL_GetKeyboardState(nullptr);

	movementDirection = glm::vec3(0);
	if (state[SDL_SCANCODE_W])
		camera.pos += glm::normalize(forward) * deltaTime * multiplier;
	else if (state[SDL_SCANCODE_S])
		camera.pos -= glm::normalize(forward) * deltaTime * multiplier;
	if (state[SDL_SCANCODE_A])
		camera.pos += right * deltaTime * multiplier;
	else if (state[SDL_SCANCODE_D])
		camera.pos -= right * deltaTime * multiplier;
	if (state[SDL_SCANCODE_SPACE])
		camera.pos += up * deltaTime * multiplier;
	else if (state[SDL_SCANCODE_LSHIFT])
		camera.pos -= up * deltaTime * multiplier;
	if (state[SDL_SCANCODE_R])
		multiplier += 10;

	while (SDL_PollEvent(&event)) {
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
			if (event.key.keysym.sym == SDLK_F11) {
				if (fullscreen != true) {
					fullscreen = true;
					window_width = 1920;
					window_height = 1080;
					SDL_SetWindowSize(window, window_width, window_height);
					SDL_SetWindowFullscreen(window, SDL_TRUE);
					recreateSwapchain();
					recreateRenderPass();
				}
				else {
					fullscreen = false;
					window_width = 1280;
					window_height = 720;
					SDL_SetWindowSize(window, window_width, window_height);
					SDL_SetWindowFullscreen(window, SDL_FALSE);
					recreateSwapchain();
					recreateRenderPass();
				}
			}
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

	{
		VkDescriptorPoolSize poolSizes[] = {
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, FRAMES_IN_FLIGHT },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, FRAMES_IN_FLIGHT },
		};
		VkDescriptorPoolCreateInfo createInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
		createInfo.maxSets = FRAMES_IN_FLIGHT;
		createInfo.poolSizeCount = ARRAY_COUNT(poolSizes);
		createInfo.pPoolSizes = poolSizes;
		VKA(vkCreateDescriptorPool(context->device, &createInfo, 0, &uniformDescriptorPool));
	}

	createTriangles(&triangles, "..\\resources\\monke.obj");

	for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
		// Very long ;)
		createBuffer(context, &uniformBuffers[i], sizeof(UniformBufferObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		createBuffer(context, &listBuffers[i], sizeof(Triangle) * triangles.size(), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	}

	{
		VkDescriptorSetLayoutBinding bindings[] = {
			{ 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, 0 },
			{ 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, 0 },
		};
		VkDescriptorSetLayoutCreateInfo createInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
		createInfo.bindingCount = ARRAY_COUNT(bindings);
		createInfo.pBindings = bindings;
		VKA(vkCreateDescriptorSetLayout(context->device, &createInfo, 0, &descriptorSetLayout));

		UniformBufferObject ubo = { camera.pos, rotationMatrix, glm::ivec2(window_width, window_height), firstFrame };

		for (uint32_t i = 0; i < triangles.size(); i++) {
			printf("V0: %.2f, %.2f, %.2f\n", triangles[i].v0.x, triangles[i].v0.y, triangles[i].v0.z);
			printf("V1: %.2f, %.2f, %.2f\n", triangles[i].v1.x, triangles[i].v1.y, triangles[i].v1.z);
			printf("V2: %.2f, %.2f, %.2f\n", triangles[i].v2.x, triangles[i].v2.y, triangles[i].v2.z);
		}

		for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
			VkDescriptorSetAllocateInfo allocateInfo = {};
			allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			allocateInfo.descriptorPool = uniformDescriptorPool;
			allocateInfo.descriptorSetCount = 1;
			allocateInfo.pSetLayouts = &descriptorSetLayout;
			VKA(vkAllocateDescriptorSets(context->device, &allocateInfo, &uniformDescriptorSets[i]));

			VkDescriptorBufferInfo bufferInfo = { uniformBuffers[i].buffer, 0, sizeof(UniformBufferObject) };
			VkWriteDescriptorSet descriptorWrites[2];
			descriptorWrites[0] = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
			descriptorWrites[0].dstSet = uniformDescriptorSets[i];
			descriptorWrites[0].dstBinding = 0;
			descriptorWrites[0].descriptorCount = 1;
			descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrites[0].pBufferInfo = &bufferInfo;
			VkDescriptorBufferInfo listBufferInfo = { listBuffers[i].buffer, 0, sizeof(Triangle) * triangles.size() };
			descriptorWrites[1] = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
			descriptorWrites[1].dstSet = uniformDescriptorSets[i];
			descriptorWrites[1].dstBinding = 1;
			descriptorWrites[1].descriptorCount = 1;
			descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			descriptorWrites[1].pBufferInfo = &listBufferInfo;
			VK(vkUpdateDescriptorSets(context->device, ARRAY_COUNT(descriptorWrites), descriptorWrites, 0, 0));
		}
	}

	VkVertexInputAttributeDescription vertexAttributeDescriptions[2] = {};
	vertexAttributeDescriptions[0].binding = 0;
	vertexAttributeDescriptions[0].location = 0;
	vertexAttributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
	vertexAttributeDescriptions[0].offset = 0;
	vertexAttributeDescriptions[1].binding = 0;
	vertexAttributeDescriptions[1].location = 1;
	vertexAttributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	vertexAttributeDescriptions[1].offset = sizeof(float) * 2;
	VkVertexInputBindingDescription vertexInputBinding = {};
	vertexInputBinding.binding = 0;
	vertexInputBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	vertexInputBinding.stride = sizeof(float) * 5;
	pipeline = createPipeline(context, "../shaders/vert.spv", "../shaders/frag.spv", renderPass, swapchain.width, swapchain.height, vertexAttributeDescriptions, ARRAY_COUNT(vertexAttributeDescriptions), &vertexInputBinding, 1, &descriptorSetLayout, 0);

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

	for (uint32_t i = 0; i < ARRAY_COUNT(commandPools); i++) {
		// Sets the Command Pool
		VkCommandPoolCreateInfo createInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
		createInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
		createInfo.queueFamilyIndex = context->graphicsQueue.familyIndex;
		VKA(vkCreateCommandPool(context->device, &createInfo, 0, &commandPools[i]));
	}

	for (uint32_t i = 0; i < ARRAY_COUNT(commandPools); i++) {
		// Sets the Command Buffer
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
		VkRect2D scissor = { { 0, 0 }, { swapchain.width, swapchain.height } };
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

		VkClearValue clearValue = { 0.0f, 1.0f, 1.0f, 1.0f };
		VkRenderPassBeginInfo beginInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
		beginInfo.renderPass = renderPass;
		beginInfo.framebuffer = framebuffers[imageIndex];
		beginInfo.renderArea = { { 0, 0 }, { swapchain.width, swapchain.height } };
		beginInfo.clearValueCount = 1;
		beginInfo.pClearValues = &clearValue;
		vkCmdBeginRenderPass(commandBuffer, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipeline);

		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipelineLayout, 0, 1, &uniformDescriptorSets[frameIndex], 0, 0);

		UniformBufferObject ubo = { camera.pos, rotationMatrix, glm::ivec2(window_width, window_height), firstFrame };
		 
		void* mapped;
		VK(vkMapMemory(context->device, uniformBuffers[frameIndex].memory, 0, sizeof(ubo), 0, &mapped));
		memcpy(mapped, &ubo, sizeof(ubo));
		VK(vkUnmapMemory(context->device, uniformBuffers[frameIndex].memory));

		VK(vkMapMemory(context->device, listBuffers[frameIndex].memory, 0, VK_WHOLE_SIZE, 0, &mapped));
		memcpy(mapped, triangles.data(), sizeof(Triangle) * triangles.size());
		VK(vkUnmapMemory(context->device, listBuffers[frameIndex].memory));

		VkDeviceSize offset = 0;
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer.buffer, &offset);
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipeline);
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipelineLayout, 0, 1, &uniformDescriptorSets[frameIndex], 0, 0);

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

	VK(vkDestroyDescriptorPool(context->device, uniformDescriptorPool, 0));
	VK(vkDestroyDescriptorSetLayout(context->device, descriptorSetLayout, 0));

	destroyBuffer(context, &vertexBuffer);

	for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
		destroyBuffer(context, &uniformBuffers[i]);
		destroyBuffer(context, &listBuffers[i]);
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
	camera.pos = glm::vec3(0.0f, 0.0f, 2.0f);
	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		LOG_ERROR("Error initializing SDL: ", SDL_GetError());
		return 1;
	}

	SDL_Window* window = SDL_CreateWindow(
		"Vulkan Real-Time Ray Tracer", 
		SDL_WINDOWPOS_CENTERED, 
		SDL_WINDOWPOS_CENTERED, 
		window_width, 
		window_height, 
		SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE
	);
	if (!window) {
		LOG_ERROR("Error creating SDL window: ", SDL_GetError());
		return 1;
	}

	initApp(window);

	while (handleMessage(window, window_width, window_height)) {
		currentFrame = (float)(SDL_GetTicks()) / 1000.0f;
		firstFrame += currentFrame;

		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;
		renderApp();
	}

	shutdownApp(window);

	return 0;
}