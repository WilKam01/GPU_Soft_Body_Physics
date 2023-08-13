#pragma once

#include "Descriptors.h"
#include "resources/Mesh.h"

struct PipelineSettings
{
	VkPolygonMode polygonMode;
	bool backfaceCulling;
	bool depthTesting;
};

class PipelineLayout
{
private:
	Device* p_device;
	DescriptorSetLayout* p_descriptorSetLayout;
	VkPipelineLayout m_layout;
	VkShaderStageFlags m_pushConstantShaderStage;
public:
	void init(Device& device, DescriptorSetLayout* layout = nullptr, uint32_t pushConstantSize = 0, VkShaderStageFlags pushConstantShaderStage = VK_SHADER_STAGE_FRAGMENT_BIT);
	void cleanup();

	void bindDescriptors(VkCommandBuffer commandBuffer, VkPipelineBindPoint bindPoint, const std::vector<VkDescriptorSet>& descriptorSets);
	void pushConstants(VkCommandBuffer commandBuffer, uint32_t size, const void* data);

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
		VkRenderPass renderPass,
		const std::string& vertexShaderPath,
		const std::string& fragmentShaderPath,
		PipelineSettings settings,
		VertexStreamInput inputStreams,
		VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
	);
	void initGraphics(
		Device& device,
		PipelineLayout& layout,
		VkRenderPass renderPass,
		const std::string& vertexShaderPath,
		PipelineSettings settings,
		VertexStreamInput inputStreams
	);
	void initCompute(
		Device& device,
		PipelineLayout& layout,
		const std::string& shaderPath
	);

	void cleanup();

	inline VkPipeline get() { return m_pipeline; }
};

