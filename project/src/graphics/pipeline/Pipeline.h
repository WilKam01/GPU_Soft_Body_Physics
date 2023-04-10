#pragma once

#include "Descriptors.h"

struct Vertex
{
	glm::vec3 position;
	float pad0;
	glm::vec3 color;
	float pad1;
};

class PipelineLayout
{
private:
	Device* p_device;
	DescriptorSetLayout* p_descriptorSetLayout;
	VkPipelineLayout m_layout;
public:
	void init(Device& device, DescriptorSetLayout* layout = nullptr);
	void cleanup();

	inline VkPipelineLayout get() { return m_layout; }
};

class Pipeline
{
private:
	Device* p_device;
	PipelineLayout* p_layout;
	DescriptorSet* p_descriptorSet;

	VkPipeline m_pipeline;
	VkPipelineBindPoint m_bindPoint;

	VkShaderModule loadShader(const std::string& path);
public:
	void initGraphics(
		Device& device,
		PipelineLayout& layout,
		DescriptorSet* descriptorSet,
		VkRenderPass renderPass,
		const std::string& vertexShaderPath,
		const std::string& fragmentShaderPath
	);
	void initCompute(
		Device& device,
		PipelineLayout& layout,
		DescriptorSet* descriptorSet,
		const std::string& shaderPath
	);

	void cleanup();



	inline VkPipeline get() { return m_pipeline; }
};

