#include <sumire/core/models/mesh.hpp>

namespace sumire {

	Mesh::Mesh(SumiDevice &device, glm::mat4 matrix) {
		// Create Uniform Buffer
		uniforms.matrix = matrix;
		
		// Create mesh uniform buffer and descriptor
		// TODO: Maybe use host coherent here for ease of update
		uniformBuffer = std::make_unique<SumiBuffer>(
			device,
			sizeof(Mesh::UniformData),
			1,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
		);
		uniformBuffer->map();
		uniformBuffer->writeToBuffer(&uniforms); // initial uniform buffer write

		// Descriptor is left unanitialized until the model initializes it.
	}

	void Mesh::initJointBuffer(SumiDevice &device, uint32_t nJoints) {
		// Leave buffer as VK_NULL_HANDLE if no joints present
		if (nJoints <= 0) return;

		// Create Joint SSBO
		jointBuffer = std::make_unique<SumiBuffer>(
			device,
			nJoints * sizeof(Mesh::JointData),
			1,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
		);
		jointBuffer->map();
	}

}