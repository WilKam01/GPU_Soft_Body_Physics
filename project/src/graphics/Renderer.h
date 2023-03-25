#pragma once

#include "Instance.h"
#include "Device.h"
#include "SwapChain.h"
#include "CommandPool.h"
#include "CommandBufferArray.h"

class Renderer
{
public:
	const int MAX_FRAMES_IN_FLIGHT = 2;
private:
	uint32_t currentFrame;

	Instance m_instance;
	Device m_device;
	SwapChain m_swapChain;
	VkSurfaceKHR m_surface;

	VkPipelineLayout m_pipelineLayout;
	VkPipeline m_graphicsPipeline;
	VkViewport m_viewport;
	VkRect2D m_scissor;

	CommandPool m_commandPool;
	CommandBufferArray m_commandBufferArray;

	std::vector<VkSemaphore> m_imageAvailableSemaphores;
	std::vector<VkSemaphore> m_renderFinishedSemaphores;
	std::vector<VkFence> m_inFlightFences;

	void createPipelineLayout();
	void createGraphicsPipeline();
	void createCommandBuffers();
	void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
	void createSyncObjects();

	void recreateSwapChain();
public:
	void init(Window& window);
	void cleanup();
	void render();
};

