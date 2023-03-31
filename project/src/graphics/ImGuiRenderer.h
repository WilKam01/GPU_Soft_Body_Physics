#pragma once

#include "Device.h"
#include "SwapChain.h"
#include "CommandPool.h"

class ImGuiRenderer
{
private:
	Device* p_device;
	SwapChain* p_swapChain;
	Buffer* p_vertexBuffer;

	VkRenderPass m_renderPass;
	VkDescriptorPool m_descriptorPool;
	std::vector<VkFramebuffer> m_framebuffers;

	void createRenderPass();
	void createFramebuffers();
public:
	void init(Window& window, Instance& instance, Device& device, SwapChain& swapChain, CommandPool& commandPool, Buffer* vertexBuffer);
	void cleanup();
	void recreateFramebuffers();

	void render();

	inline VkRenderPass getRenderPass() { return m_renderPass; }
	inline VkFramebuffer getFramebuffer(size_t i) { return m_framebuffers[i]; }
};

