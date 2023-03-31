#pragma once

#include "Instance.h"
#include "Device.h"
#include "SwapChain.h"
#include "CommandPool.h"
#include "CommandBufferArray.h"
#include "Buffer.h"
#include "ImGuiRenderer.h"

struct Vertex
{
	glm::vec3 position;
	glm::vec3 color;
};

class Renderer
{
public:
	const static int MAX_FRAMES_IN_FLIGHT = 2;
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

	Buffer m_vertexBuffer;
	Buffer m_indexBuffer;

	std::vector<VkSemaphore> m_imageAvailableSemaphores;
	std::vector<VkSemaphore> m_renderFinishedSemaphores;
	std::vector<VkFence> m_inFlightFences;

	ImGuiRenderer m_imGuiRenderer;

	void createPipelineLayout();
	void createGraphicsPipeline();
	void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
	void createSyncObjects();

	void createMeshBuffers();

	void recreateSwapChain();
public:
	void init(Window& window);
	void cleanup();
	void render();
};

