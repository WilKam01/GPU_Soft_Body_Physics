#pragma once

#include "Instance.h"
#include "Device.h"
#include "SwapChain.h"
#include "CommandPool.h"
#include "CommandBufferArray.h"
#include "Buffer.h"
#include "resources/Mesh.h"
#include "pipeline/Pipeline.h"
#include "pipeline/UniformBuffer.h"
#include "pipeline/Sampler.h"
#include "ImGuiRenderer.h"
#include "resources/Importer.h"

struct Matrices 
{
	glm::mat4 model;
	glm::mat4 viewProj;
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

	PipelineLayout m_graphicsPipelineLayout;
	Pipeline m_graphicsPipeline;

	DescriptorSetLayout m_graphicsDescriptorSetLayout;
	DescriptorSet m_graphicsDescriptorSet;
	DescriptorSet m_meshDescriptorSet;
	DescriptorSet m_floorDescriptorSet;

	PipelineLayout m_computePipelineLayout;
	Pipeline m_computePipeline;
	DescriptorSetLayout m_computeDescriptorSetLayout;
	DescriptorSet m_computeDescriptorSet;

	Texture m_texture;
	Sampler m_sampler;
	Mesh m_mesh;

	Texture m_floorTexture;
	Mesh m_floorMesh;

	VkViewport m_viewport;
	VkRect2D m_scissor;

	CommandPool m_commandPool;
	CommandBufferArray m_commandBufferArray;
	CommandPool m_computeCommandPool;
	CommandBufferArray m_computeCommandBufferArray;

	std::vector<UniformBuffer<Matrices>> m_matricesUBO;
	std::vector<UniformBuffer<float>> m_deltaTimeUBO;

	std::vector<VkSemaphore> m_imageAvailableSemaphores;
	std::vector<VkSemaphore> m_renderFinishedSemaphores;
	std::vector<VkFence> m_inFlightFences;

	std::vector<VkFence> m_computeInFlightFences;
	std::vector<VkSemaphore> m_computeFinishedSemaphores;

	ImGuiRenderer m_imGuiRenderer;

	void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
	void recordCommandBufferCompute(VkCommandBuffer commandBuffer);
	void createSyncObjects();

	void createResources();

	void recreateSwapChain();
	void renderImGui();
public:
	void init(Window& window);
	void cleanup();
	void render();
};

