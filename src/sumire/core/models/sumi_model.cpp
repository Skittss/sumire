#include <sumire/core/models/sumi_model.hpp>
#include <sumire/core/render_systems/data_structs/mesh_rendersys_structs.hpp>

#include <sumire/core/rendering/sumi_swap_chain.hpp>

// TODO: Could we find a way around using experimental GLM hashing? (though it seems stable)
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

#include <cassert>
#include <cstring>
#include <iostream>

namespace sumire {

	// Creates a model. 
	// Note: loader data is *moved* after being passed to this constructor and should NOT be reaccessed.
	SumiModel::SumiModel(SumiDevice &device, SumiModel::Data &data) 
		: sumiDevice{ device }
	{
		// Move data from loader struct into member variables.
		//  This is necessary as loader struct contains unique_ptrs, 
		//  as well as being generally more memory-efficient (No temp vars required while moving large arrays)
		nodes = std::move(data.nodes);
		flatNodes = std::move(data.flatNodes);
		meshCount = std::move(data.meshCount);
		skins = std::move(data.skins);
		animations = std::move(data.animations);
		materials = std::move(data.materials);

		// Init resources on the GPU
		createVertexBuffers(data.vertices);
		createIndexBuffer(data.indices);
		createDefaultTextures();
		initDescriptors();
		createMaterialStorageBuffer();
	}

	SumiModel::~SumiModel() {
		materialDescriptorPool = nullptr;
		meshNodeDescriptorPool = nullptr;
		indexBuffer = nullptr;
		vertexBuffer = nullptr;
	}

	void SumiModel::createVertexBuffers(const std::vector<Vertex>& vertices) {
		vertexCount = static_cast<uint32_t>(vertices.size());
		assert(vertexCount >= 3 && "Vertex count must be at least 3");
		uint32_t vertexInstanceSize = sizeof(vertices[0]);
		VkDeviceSize bufferSize = vertexInstanceSize * vertexCount; // vb size

		SumiBuffer stagingBuffer{
			sumiDevice,
			vertexInstanceSize,
			vertexCount,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			// Allow CPU to write to the staging buffer, which is then automatically flushed to the GPU (coherent bit)
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
		};

		stagingBuffer.map();
		stagingBuffer.writeToBuffer((void *)vertices.data());

		vertexBuffer = std::make_unique<SumiBuffer>(
			sumiDevice,
			vertexInstanceSize,
			vertexCount,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			// use fast local GPU memory.
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
		);

		sumiDevice.copyBuffer(stagingBuffer.getBuffer(), vertexBuffer->getBuffer(), bufferSize);
	}

	void SumiModel::createIndexBuffer(const std::vector<uint32_t>& indices) {
		indexCount = static_cast<uint32_t>(indices.size());
		useIndexBuffer = indexCount > 0;

		if (!useIndexBuffer) return; 
		
		uint32_t indexInstanceSize = sizeof(indices[0]);
		VkDeviceSize bufferSize = indexInstanceSize * indexCount; // ib size
		
		SumiBuffer stagingBuffer{
			sumiDevice,
			indexInstanceSize,
			indexCount,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			// Allow CPU to write to the staging buffer, which is then automatically flushed to the GPU (coherent bit)
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
		};

		stagingBuffer.map();
		stagingBuffer.writeToBuffer((void *)indices.data());

		indexBuffer = std::make_unique<SumiBuffer>(
			sumiDevice,
			indexInstanceSize,
			indexCount,
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			// use fast local GPU memory.
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
		);

		sumiDevice.copyBuffer(stagingBuffer.getBuffer(), indexBuffer->getBuffer(), bufferSize);
	}

	void SumiModel::createDefaultTextures() {
		// Empty texture
		VkImageCreateInfo imageInfo{};
		SumiTexture::defaultImageCreateInfo(imageInfo);
		VkSamplerCreateInfo samplerInfo{};
		SumiTexture::defaultSamplerCreateInfo(sumiDevice, samplerInfo);

		// TODO: Make this texture compressed (KTX / DDS) if possible
		emptyTexture = SumiTexture::createFromFile(
			sumiDevice,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			imageInfo,
			samplerInfo,
			"../assets/textures/empty.png"
		);
	}

	void SumiModel::initDescriptors() {

		// == Mesh Nodes ====================================================================
		// - To push local transform matrices to the shader via UBO

		// Per-Node descriptor pool
		meshNodeDescriptorPool = SumiDescriptorPool::Builder(sumiDevice)
			.setMaxSets(
				1 + meshCount * SumiSwapChain::MAX_FRAMES_IN_FLIGHT)
			// Local matrices
			.addPoolSize(
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 
				meshCount * SumiSwapChain::MAX_FRAMES_IN_FLIGHT)
			// Skinning information
			.addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1)
			.build();

		// Nodes descriptor set layout
		auto meshNodeDescriptorSetLayout = SumiModel::meshNodeDescriptorLayout(sumiDevice);

		// Per-Node Descriptor Sets for local matrices
		//   Iterate flat nodes to skip doing recursion here on children.
		for (auto &node : flatNodes) {
			if (node->mesh) {
				auto uniformBufferInfo = node->mesh->uniformBuffer->descriptorInfo();
				//auto jointBufferInfo = node->mesh->jointBuffer->descriptorInfo();
				auto writer = SumiDescriptorWriter(*meshNodeDescriptorSetLayout, *meshNodeDescriptorPool);
				writer.writeBuffer(0, &uniformBufferInfo);
				if (node->mesh->jointBuffer) {
					auto jointBufferInfo = node->mesh->jointBuffer->descriptorInfo();
					writer.writeBuffer(1, &jointBufferInfo);
				}
				writer.build(node->mesh->descriptorSet);

					//.writeBuffer(1, &jointBufferInfo)
					//.build(node->mesh->descriptorSet);
			}
		}

		// == Materials =====================================================================
		// - To push textures and material factors to the shader via CIS and UBO

		// TODO: Offload this descriptor pool to a material manager class for the whole scene.
		materialDescriptorPool = SumiDescriptorPool::Builder(sumiDevice)
			.setMaxSets(
				1 + (materials.size() * SumiMaterial::MAT_TEX_COUNT) * SumiSwapChain::MAX_FRAMES_IN_FLIGHT)
			// Texture samplers
			.addPoolSize(
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 
				materials.size() * SumiMaterial::MAT_TEX_COUNT * SumiSwapChain::MAX_FRAMES_IN_FLIGHT)
			// Single SSBO for materials
			.addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1)
			.build();

		// Read in Descriptor Set Layout from material class
		auto materialDescriptorLayout = SumiMaterial::getDescriptorSetLayout(sumiDevice);

		// Per-Material Descriptor Sets for Texture Samplers
		for (auto& mat : materials) {
			// Write to images
			mat->writeDescriptorSet(*materialDescriptorPool, *materialDescriptorLayout, emptyTexture.get());
		}
	}

	void SumiModel::createMaterialStorageBuffer() {

		// Gather material info
		std::vector<SumiMaterial::MaterialShaderData> matShaderData;
		for (auto& mat : materials) {
			matShaderData.push_back(mat->getMaterialShaderData());
		}

		VkDeviceSize bufferSize = matShaderData.size() * sizeof(SumiMaterial::MaterialShaderData);

		// Create Storage Buffer
		materialStorageBuffer = std::make_unique<SumiBuffer>(
			sumiDevice,
			bufferSize,
			1,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
		);

		// Descriptor set layout
		auto matStorageDescriptorLayout = SumiModel::matStorageDescriptorLayout(sumiDevice);

		// Material Storage Buffer Descriptor
		auto bufferInfo = materialStorageBuffer->descriptorInfo();
		SumiDescriptorWriter(*matStorageDescriptorLayout, *materialDescriptorPool)
			.writeBuffer(0, &bufferInfo)
			.build(materialStorageDescriptorSet);

		// Stage and write to device local memory
		SumiBuffer stagingBuffer{
			sumiDevice,
			bufferSize,
			1,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
		};
		stagingBuffer.map();
		stagingBuffer.writeToBuffer((void *)matShaderData.data());

		sumiDevice.copyBuffer(stagingBuffer.getBuffer(), materialStorageBuffer->getBuffer(), bufferSize);
	}

	std::unique_ptr<SumiDescriptorSetLayout> SumiModel::meshNodeDescriptorLayout(SumiDevice &device) {
		return SumiDescriptorSetLayout::Builder(device)
			.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
			// Joint storage buffer can be partially bound (i.e. potentially null)
			.addBinding(
				1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 1,
				VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT
			)
			.build();
	}

	std::unique_ptr<SumiDescriptorSetLayout> SumiModel::matTextureDescriptorLayout(SumiDevice &device) {
		return SumiMaterial::getDescriptorSetLayout(device);
	}

	std::unique_ptr<SumiDescriptorSetLayout> SumiModel::matStorageDescriptorLayout(SumiDevice &device) {
		return SumiDescriptorSetLayout::Builder(device)
			.addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT)
			.build();
	}

	void SumiModel::bind(VkCommandBuffer commandBuffer) {
		// Vertex and Index Buffers
		VkBuffer buffers[] = { vertexBuffer->getBuffer() };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);

		if (useIndexBuffer) {
			vkCmdBindIndexBuffer(commandBuffer, indexBuffer->getBuffer(), 0, VK_INDEX_TYPE_UINT32);
		}
	}

	void SumiModel::drawNode(
		Node *node, 
		VkCommandBuffer commandBuffer, 
		VkPipelineLayout pipelineLayout,
		const std::unordered_map<SumiPipelineStateFlags, std::unique_ptr<SumiPipeline>> &pipelines
	) {
		// Draw this node's primitives
		if (node->mesh) {
			for (auto& primitive : node->mesh->primitives) {

				// Bind required pipeline
				pipelines.at(primitive->material->requiredPipelineState)->bind(commandBuffer);

				// Bind descriptor sets
				const std::vector<VkDescriptorSet> descriptorSets{
					primitive->material->getDescriptorSet(),
					node->mesh->descriptorSet,
					materialStorageDescriptorSet,
				};

				// Fragment Shader Push constants (for material indexing)
				structs::FragPushConstantData push{};
				push.materialIdx = primitive->materialIdx;

				vkCmdPushConstants(
					commandBuffer,
					pipelineLayout,
					VK_SHADER_STAGE_FRAGMENT_BIT,
					sizeof(structs::VertPushConstantData),
					sizeof(structs::FragPushConstantData),
					&push
				);

				// IMPORTANT: Reserve set 0 for global descriptors
				vkCmdBindDescriptorSets(
					commandBuffer,
					VK_PIPELINE_BIND_POINT_GRAPHICS,
					pipelineLayout,
					1,
					static_cast<uint32_t>(descriptorSets.size()),
					descriptorSets.data(),
					0, nullptr
				);
				
				// Draw
				if (primitive->indexCount > 0) {
					vkCmdDrawIndexed(commandBuffer, primitive->indexCount, 1, primitive->firstIndex, 0, 0);
				} else {
					vkCmdDraw(commandBuffer, primitive->vertexCount, 1, 0, 0);
				}
			}
		}

		// Draw children
		for (auto& child : node->children) {
			drawNode(child, commandBuffer, pipelineLayout, pipelines);
		}
	}

	// Draw a model node tree. *Starts binding descriptors from set 1*
	void SumiModel::draw(
		VkCommandBuffer commandBuffer, 
		VkPipelineLayout pipelineLayout,
		const std::unordered_map<SumiPipelineStateFlags, std::unique_ptr<SumiPipeline>> &pipelines
	) {
		for (auto& node : nodes) {
			drawNode(node, commandBuffer, pipelineLayout, pipelines);
		}
	}

	// Update a range of animations for this model.
	void SumiModel::updateAnimations(const std::vector<uint32_t> indices, float time, bool loop) {
		if (animations.empty() || indices.empty()) return;

		for (const uint32_t &i : indices) {
			updateAnimation(i, time, loop);
		}
	}

	void SumiModel::updateAnimation(uint32_t animIdx, float time, bool loop) {
		assert(animIdx < animations.size() && animIdx >= 0 && "Animation index out of range");
		
		std::unique_ptr<Animation>& animation = animations[animIdx];

		// loop animation
		// TODO: I'm not sure this works if time < animation->start
		//		 it would be better to use math::fmod here
		if (loop && time > animation->end) {
			float duration = animation->end - animation->start;
			float exceeds = time - animation->start - duration;
			float n_times_exceeds = std::floor(exceeds/duration);
			time = animation->start + (exceeds - n_times_exceeds * duration);
		}

		// TODO: Perhaps this "changed" flag should be given to each node so only updated nodes are affected.
		bool modelUpdated = false;
		for (auto& channel : animation->channels) {
			AnimationSampler &sampler = animation->samplers[channel.samplerIdx];
			
			// Note: Only loop to penultimate animation state as we interpolate between keyframes
			//		 i and i + 1.
			for (size_t i = 0; i < sampler.inputs.size() - 1; i++) {
				
				// current keyframe (0) and next keyframe (1) input data
				float input0 = sampler.inputs[i];
				float input1 = sampler.inputs[i + 1];

				// Only update this animation when we are within its specified time-frame
				if (time < input0 || time > input1) continue;

				// Interpolation value
				float u = std::max(0.0f, time - input0) / (input1 - input0);
				if (u < 0.0f || u > 1.0f) continue;

				// current keyframe (0) and next keyframe (1) output data
				glm::vec4 output0;
				glm::vec4 output1;
				// (output data to fill if cubic spline):
				glm::vec4 inTangent0{0.0f};
				glm::vec4 outTangent0{0.0f};
				glm::vec4 inTangent1{0.0f};
				glm::vec4 outTangent1{0.0f};

				// Fill output data
				switch (sampler.interpolation) {
					case util::INTERP_STEP:
					case util::INTERP_LINEAR: {
						output0 = sampler.outputs[i];
						output1 = sampler.outputs[i + 1];
					}
					break;
					case util::INTERP_CUBIC_SPLINE: {
						// Cublic spline data is formatted as (in tangent, value, out tangent) triplets.
						float offset0 = 3*i;
						float offset1 = 3*(i+1);
						inTangent0  = sampler.outputs[offset0];
						output0     = sampler.outputs[offset0 + 1];
						outTangent0 = sampler.outputs[offset0 + 2];
						inTangent1  = sampler.outputs[offset1];
						output1     = sampler.outputs[offset1 + 1];
						outTangent1 = sampler.outputs[offset1 + 2];
					}
					break;
				}

				// Update pointed mesh node parameters (T,S,R,W)
				switch (channel.path) {
					case AnimationChannel::PathType::TRANSLATION: {
						glm::vec4 translation = util::interpVec4(output0, outTangent0, output1, inTangent1, u, sampler.interpolation);;
						channel.node->setTranslation(glm::vec3(translation));
					}
					break;
					case AnimationChannel::PathType::SCALE: {
						glm::vec4 scale = util::interpVec4(output0, outTangent0, output1, inTangent1, u, sampler.interpolation);
						channel.node->setScale(glm::vec3(scale));
					}
					break;
					case AnimationChannel::PathType::ROTATION: {
						glm::quat q0;
						q0.x = output0.x; 
						q0.y = output0.y; 
						q0.z = output0.z; 
						q0.w = output0.w;
						glm::quat q0_ot = glm::quat{outTangent0.x, outTangent0.y, outTangent0.z, outTangent0.w};
						q0_ot.x = outTangent0.x; 
						q0_ot.y = outTangent0.y;
						q0_ot.x = outTangent0.z; 
						q0_ot.y = outTangent0.w;
						glm::quat q1;
						q1.x = output1.x; 
						q1.y = output1.y; 
						q1.z = output1.z; 
						q1.w = output1.w;
						glm::quat q1_it = glm::quat(inTangent1.x, inTangent1.y, inTangent1.z, inTangent1.w);
						q1_it.x = inTangent1.x; 
						q1_it.y = inTangent1.y; 
						q1_it.z = inTangent1.z; 
						q1_it.w = inTangent1.w;

						channel.node->setRotation(util::interpQuat(q0, q0_ot, q1, q1_it, u, sampler.interpolation));
					}
					break;
					case AnimationChannel::PathType::WEIGHTS: {
						std::runtime_error("TODO: Morph target animation update via weights.");
					}
					break;
				}

				modelUpdated = true;
			}
		}

		// Only update nodes if we advanced any animation channels
		if (modelUpdated) {
			updateNodes();
		}
	}

	void SumiModel::updateNodes() {
		for (auto& node : nodes) {
			node->applyTransformHierarchy();
			node->updateRecursive();
		}
	}

}