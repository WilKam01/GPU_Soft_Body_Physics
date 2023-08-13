#pragma once

#include "pch.h"
#include "graphics/renderers/Renderer.h"

struct SoftBody
{
	Mesh mesh;
	TetrahedralMesh tetMesh;

	DescriptorSet graphicsDescriptorSet;
	DescriptorSet graphicsSurfaceDescriptorSet;
	DescriptorSet pbdDescriptorSet;
	DescriptorSet colDescriptorSet;
	DescriptorSet deformDescriptorSet;

	// Buffer used to deform the original mesh, either directly in the form of indices or in the form of tetrahedral deformation
	Buffer deformBuffer;

	// Collision buffers
	std::array<Buffer, Renderer::MAX_FRAMES_IN_FLIGHT> colSizeBuffer;
	std::array<Buffer, Renderer::MAX_FRAMES_IN_FLIGHT> colConstraintBuffer;

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
			for (int i = 0; i < Renderer::MAX_FRAMES_IN_FLIGHT; i++)
			{
				colConstraintBuffer[i].cleanup();
				colSizeBuffer[i].cleanup();
			}
			deformBuffer.cleanup();
			deformDescriptorSet.cleanup();
			colDescriptorSet.cleanup();
			pbdDescriptorSet.cleanup();
			graphicsSurfaceDescriptorSet.cleanup();
			graphicsDescriptorSet.cleanup();
			deformUBO.cleanup();
			pbdUBO.cleanup();
			tetMesh.cleanup();
			mesh.cleanup();
		}
		active = false;
	}
};