#pragma once

#include "../Device.h"
#include "../SwapChain.h"
#include "../CommandPool.h"
#include "../CommandBufferArray.h"
#include "../pipeline/Pipeline.h"
#include "../pipeline/RenderArea.h"

class ShadowRenderer
{
private:
	Device* p_device;
	SwapChain* p_swapChain;
	CommandPool* p_commandPool;

	Texture m_depthTexture;
	VkRenderPass m_renderPass;
	VkFramebuffer m_framebuffer;

	PipelineLayout m_pipelineLayout;
	Pipeline m_pipeline;

	uint32_t m_size;
	RenderArea m_renderArea;

	void createRenderPass();
	void createFramebuffer();
	void createPipeline();
public:
	void init(Device& device, SwapChain& swapChain, CommandPool& commandPool, uint32_t size);
	void cleanup();

	void bind(VkCommandBuffer commandBuffer, glm::mat4 matrix);
	
	inline Texture& getDepthTexture() { return m_depthTexture; }
};

