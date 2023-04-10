#pragma once

#include "Device.h"

struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

class SwapChain
{
private:
	VkSwapchainKHR m_swapChain;
	std::vector<VkImage> m_images;
	std::vector<VkImageView> m_imageViews;
	std::vector<VkFramebuffer> m_framebuffers;
	VkFormat m_imageFormat;
	VkExtent2D m_extent;
	VkRenderPass m_renderPass;

	Device* p_device;
	VkSurfaceKHR m_surface;
	Window* p_window;

	VkSurfaceFormatKHR chooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR choosePresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
	VkExtent2D chooseExtent(const VkSurfaceCapabilitiesKHR& capabilities);

	void create();
	void createImageViews();
	void createRenderPass();
	void createFramebuffers();
public:
	void init(Device& device, VkSurfaceKHR surface, Window& window);
	void cleanup();
	void recreate();
	const bool needsRecreation(); const

	static SwapChainSupportDetails querySupport(VkPhysicalDevice device, VkSurfaceKHR surface);
	inline VkSwapchainKHR get() { return m_swapChain; }
	inline VkFormat getFormat() { return m_imageFormat; }
	inline VkExtent2D getExtent() { return m_extent; }
	inline int getImageCount() { return (int)m_imageViews.size(); }
	inline VkRenderPass getRenderPass() { return m_renderPass; }

	inline VkImageView getImageView(size_t i) { return m_imageViews[i]; }
	inline VkFramebuffer getFramebuffer(size_t i) { return m_framebuffers[i]; }
};

