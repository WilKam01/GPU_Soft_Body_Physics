#pragma once

#include "../Instance.h"
#include "../Device.h"
#include "../SwapChain.h"
#include "../CommandPool.h"
#include "../CommandBufferArray.h"
#include "../Buffer.h"
#include "resources/Mesh.h"
#include "../pipeline/Pipeline.h"
#include "../pipeline/UniformBuffer.h"
#include "../pipeline/Sampler.h"
#include "../pipeline/RenderArea.h"
#include "ImGuiRenderer.h"
#include "ShadowRenderer.h"
#include "resources/ResourceManager.h"
#include "core/Timer.h"
#include "core/Camera.h"

struct MatricesUBO
{
	glm::mat4 viewProj;
	glm::mat4 light;
};

struct GraphicsUBO 
{
	glm::vec3 camPos;
	float lightIntensity;
	glm::vec3 lightDir;
	float ambient;
	float fresnel;
	float shadowAlpha;
};

struct PbdUBO
{
	float deltaTime;
	float edgeCompliance;
	float volumeCompliance;
};

struct Material
{
	glm::vec3 tint;
	float roughness;
	float metallic;
};

struct ColDetectionUBO
{
	float deltaTime;
	uint32_t triCount;
};

struct ColConstraint
{
	alignas(16) glm::vec3 orig;
	alignas(4) uint32_t particleIndex;
	alignas(16) glm::vec3 normal;
};

struct SoftBody
{
	Mesh mesh;
	TetrahedralMesh tetMesh;

	DescriptorSet graphicsDescriptorSet;
	DescriptorSet pbdDescriptorSet;
	DescriptorSet colDescriptorSet;
	DescriptorSet deformDescriptorSet;

	// Buffer used to deform the original mesh, either directly in the form of indices or in the form of tetrahedral deformation
	Buffer deformBuffer;

	// Collision buffers
	std::vector<Buffer> colSizeBuffer;
	std::vector<Buffer> colConstraintBuffer;

	// UBO information in pbd and deform shaders
	UniformBuffer<glm::uvec3> pbdUBO; // (particleCount, edgeCount, tetrahedralCount)
	UniformBuffer<glm::uvec2> deformUBO; // (vertexCount, indexCount)

	bool active = false;
	bool useTetDeformation = false;
	glm::vec3 color;

	void cleanup()
	{
		if (active)
		{
			for (int i = 0, len = (int)colConstraintBuffer.size(); i < len; i++)
			{
				colConstraintBuffer[i].cleanup();
				colSizeBuffer[i].cleanup();
			}
			deformBuffer.cleanup();
			deformDescriptorSet.cleanup();
			colDescriptorSet.cleanup();
			pbdDescriptorSet.cleanup();
			graphicsDescriptorSet.cleanup();
			deformUBO.cleanup();
			pbdUBO.cleanup();
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
	const static int MAX_SOFT_BODY_COUNT = 50;
	const static int MAX_FRAME_MEASUREMENT_COUNT = 1000;
	const static int MAX_COLLISION_CONSTRAINT_COUNT = 10000;

	const static int COLOR_COUNT = 7;
	inline const static glm::vec3 COLORS[COLOR_COUNT] = 
	{
		glm::vec3(1.0f, 0.15f, 0.05f),
		glm::vec3(0.5f, 0.15f, 0.55f),
		glm::vec3(0.9f, 0.85f, 0.1f),
		glm::vec3(0.05f, 0.3f, 0.615f),
		glm::vec3(0.0f, 0.225f, 0.325f),
		glm::vec3(0.25f, 0.5f, 0.1f),
		glm::vec3(0.65f, 0.3f, 0.0f),
	};
private:
	uint32_t currentFrame;
	Timer m_timer;
	Camera m_camera;
	ResourceManager m_resources;

	int m_fixedTimeStep = 60;
	int m_subSteps = 20;
	bool m_renderTetMesh = false;

	Instance m_instance;
	Device m_device;
	SwapChain m_swapChain;
	VkSurfaceKHR m_surface;

	RenderArea m_renderArea;
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
	Pipeline m_stretchConstraintPipeline;
	Pipeline m_volumeConstraintPipeline;
	Pipeline m_postsolvePipeline;
	DescriptorSetLayout m_pbdDescriptorSetLayout;
	DescriptorSet m_pbdDescriptorSet;

	PipelineLayout m_deformPipelineLayout;
	DescriptorSetLayout m_deformDescriptorSetLayout;
	Pipeline m_deformPipeline;
	Pipeline m_tetDeformPipeline;
	Pipeline m_recalcNormalsPipeline;
	Pipeline m_normalizeNormalsPipeline;

	uint32_t m_loadSoftBodies = 0; // Used to load soft bodies after button has been pressed, happens after old bodies have been destroyed
	std::array<SoftBody, MAX_SOFT_BODY_COUNT> m_softBodies;
	std::vector<SoftBody*> m_removeBodies; // Removed after their execution is done

	// Collision
	std::vector<UniformBuffer<ColDetectionUBO>> m_colUBO;
	Buffer m_colPositionsBuffer;
	Buffer m_colIndicesBuffer;

	PipelineLayout m_colPipelineLayout;
	DescriptorSetLayout m_colDescriptorSetLayout;
	DescriptorSet m_colDescriptorSet;
	Pipeline m_staticColDetectionPipeline;
	Pipeline m_colConstraintPipeline;

	// Measurement related
	uint32_t m_measureFrameCounter = MAX_FRAME_MEASUREMENT_COUNT;
	uint32_t m_warmupCounter = 0; // Used to wait a couple of steps when measuring the error
	bool m_measureFPS; // true : measure fps of model, false: measure error

	char m_modelName[25] = "sphere";
	int m_modelResolution = 100;
	int m_modelCount = 1;
	int m_frameCount = 250;
	glm::vec3 m_offset = glm::vec3(0.0f, 5.0f, 0.0f);

	std::vector<float> m_avgFPS;
	std::vector<float> m_avgError;
	std::vector<float> m_avgCenterError;
	std::array<std::vector<glm::vec3>, 2> m_avgCenterPos; // Average center of full res tetrahedral mesh and lower res tetrahedral mesh

	Sampler m_sampler;
	Sampler m_shadowSampler;

	Texture m_texture;
	Material m_material;
	Texture m_floorTexture;
	Material m_floorMaterial;
	Mesh m_floorMesh;

	CommandPool m_commandPool;
	CommandPool m_computeCommandPool;
	CommandBufferArray m_commandBufferArray;
	CommandBufferArray m_computeCommandBufferArray;

	std::vector<UniformBuffer<MatricesUBO>> m_matricesUBO;
	std::vector<UniformBuffer<GraphicsUBO>> m_graphicsUBO;
	std::vector<UniformBuffer<PbdUBO>> m_pbdUBO;

	std::vector<VkSemaphore> m_imageAvailableSemaphores;
	std::vector<VkSemaphore> m_renderFinishedSemaphores;
	std::vector<VkFence> m_inFlightFences;

	std::vector<VkFence> m_computeInFlightFences;
	std::vector<VkSemaphore> m_computeFinishedSemaphores;

	bool m_renderImGui = true;
	ImGuiRenderer m_imGuiRenderer;
	ShadowRenderer m_shadowRenderer;

	void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
	void detectCollisions(VkCommandBuffer commandBuffer, SoftBody& softBody);
	void computePhysics(VkCommandBuffer commandBuffer, SoftBody& softBody);
	void deformMesh(VkCommandBuffer commandBuffer, SoftBody& softBody);
	void createSyncObjects();

	void createResources();
	SoftBody createSoftBody(const std::string& name, glm::vec3 offset, int resolution = 100);

	void recreateSwapChain();
	void renderImGui();
public:
	void init(Window& window);
	void cleanup();
	void render();
};

