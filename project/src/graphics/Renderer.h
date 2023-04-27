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
#include "resources/ResourceManager.h"
#include "core/Timer.h"
#include "core/Camera.h"

struct GraphicsUBO 
{
	glm::mat4 viewProj;
	glm::vec3 camPos;
	float lightIntensity;
	glm::vec3 lightPos;
	float lightCone;
	glm::vec3 lightDir;
	float specPower;
	glm::vec4 globalAmbient;
};

struct PbdUBO
{
	float deltaTime;
	float edgeCompliance;
	float volumeCompliance;
};

struct SoftBody
{
	Mesh mesh;
	TetrahedralMesh tetMesh;
	DescriptorSet graphicsDescriptorSet;
	DescriptorSet pbdDescriptorSet;
	DescriptorSet deformDescriptorSet;
	bool useSkinning = false;

	bool active = false;
	void cleanup()
	{
		if (active)
		{
			deformDescriptorSet.cleanup();
			pbdDescriptorSet.cleanup();
			graphicsDescriptorSet.cleanup();
			tetMesh.cleanup();
			mesh.cleanup();
		}
		active = false;
	}
};

class Renderer
{
public:
	const static int MAX_FRAMES_IN_FLIGHT = 2;
private:
	uint32_t currentFrame;
	Timer m_timer;
	Camera m_camera;

	int m_fixedTimeStep = 60;
	int m_subSteps = 10;

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

	PipelineLayout m_tetPipelineLayout;
	Pipeline m_tetPipeline;
	DescriptorSetLayout m_tetDescriptorSetLayout;

	PipelineLayout m_pbdPipelineLayout;
	Pipeline m_presolvePipeline;
	Pipeline m_distanceConstraintPipeline;
	Pipeline m_volumeConstraintPipeline;
	Pipeline m_postsolvePipeline;
	DescriptorSetLayout m_pbdDescriptorSetLayout;
	DescriptorSet m_pbdDescriptorSet;

	PipelineLayout m_deformPipelineLayout;
	DescriptorSetLayout m_deformDescriptorSetLayout;
	Pipeline m_deformPipeline;

	Texture m_texture;
	Sampler m_sampler;

	SoftBody* p_currentSoftBody;
	SoftBody m_softBody;
	SoftBody m_softBody1;

	Texture m_floorTexture;
	Mesh m_floorMesh;

	VkViewport m_viewport;
	VkRect2D m_scissor;

	CommandPool m_commandPool;
	CommandBufferArray m_commandBufferArray;
	CommandPool m_computeCommandPool;
	CommandBufferArray m_computeCommandBufferArray;

	std::vector<UniformBuffer<GraphicsUBO>> m_graphicsUBO;
	std::vector<UniformBuffer<PbdUBO>> m_pbdUBO;

	std::vector<VkSemaphore> m_imageAvailableSemaphores;
	std::vector<VkSemaphore> m_renderFinishedSemaphores;
	std::vector<VkFence> m_inFlightFences;

	std::vector<VkFence> m_computeInFlightFences;
	std::vector<VkSemaphore> m_computeFinishedSemaphores;

	ImGuiRenderer m_imGuiRenderer;

	void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
	void computePhysics(VkCommandBuffer commandBuffer);
	void createSyncObjects();

	void createResources();
	SoftBody createSoftBody(const std::string& name, glm::vec3 offset);

	void recreateSwapChain();
	void renderImGui();
public:
	void init(Window& window);
	void cleanup();
	void render();
};

