#include <sumire/core/sumire.hpp>
#include <sumire/core/sumi_render_system.hpp>
#include <sumire/input/sumi_kbm_controller.hpp>
#include <sumire/core/sumi_buffer.hpp>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include <stdexcept>
#include <array>
#include <cassert>
#include <chrono>

namespace sumire {

	struct GlobalUBO {
		glm::mat4 projectionView{1.0f};
		glm::vec3 lightDir = glm::normalize(glm::vec3{1.0f, 1.0f, 1.0f});
	};

	Sumire::Sumire() {
		globalDescriptorPool = SumiDescriptorPool::Builder(sumiDevice)
			.setMaxSets(SumiSwapChain::MAX_FRAMES_IN_FLIGHT)
			.addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, SumiSwapChain::MAX_FRAMES_IN_FLIGHT)
			.build();

		loadObjects();
	}

	Sumire::~Sumire() {
		globalDescriptorPool = nullptr; // Clean up pools before device etc are cleaned up.
	}

	void Sumire::run() {

		// Make UBOs
		std::vector<std::unique_ptr<SumiBuffer>> uniformBuffers(SumiSwapChain::MAX_FRAMES_IN_FLIGHT);
		for (int i = 0; i < uniformBuffers.size(); i++) {
			uniformBuffers[i] = std::make_unique<SumiBuffer>(
				sumiDevice,
				sizeof(GlobalUBO),
				1,
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
			);
			uniformBuffers[i]->map(); //enable writing to buffer memory
		}

		auto globalDescriptorSetLayout = SumiDescriptorSetLayout::Builder(sumiDevice)
			.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
			.build();

		std::vector<VkDescriptorSet> globalDescriptorSets(SumiSwapChain::MAX_FRAMES_IN_FLIGHT);
		for (int i = 0; i < globalDescriptorSets.size(); i++) {
			auto bufferInfo = uniformBuffers[i]->descriptorInfo();
			SumiDescriptorWriter(*globalDescriptorSetLayout, *globalDescriptorPool)
				.writeBuffer(0, &bufferInfo)
				.build(globalDescriptorSets[i]);
		}

		SumiRenderSystem renderSystem{
			sumiDevice, sumiRenderer.getSwapChainRenderPass(), globalDescriptorSetLayout->getDescriptorSetLayout()
		};
		SumiCamera camera{};
		//camera.setViewTarget(glm::vec3(2.0f), glm::vec3(0.0f));

		auto viewerObj = SumiObject::createObject();
		SumiKBMcontroller cameraController{};

		auto currentTime = std::chrono::high_resolution_clock::now();

		while (!sumiWindow.shouldClose()) {
			glfwPollEvents();

			auto newTime = std::chrono::high_resolution_clock::now();
			float frameTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
			currentTime = newTime;

			const float MAX_FRAME_TIME = 0.2f; // 5fps
			frameTime = glm::min(frameTime, MAX_FRAME_TIME);

			cameraController.moveWalk(sumiWindow.getGLFWwindow(), frameTime, viewerObj);
			camera.setViewYXZ(viewerObj.transform.translation, viewerObj.transform.rotation);

			// TODO: We should not set the aspect every frame instead on change
			float aspect = sumiRenderer.getAspect();
			//camera.setOrthographicProjection(-aspect, aspect, -1.0, 1.0, -1.0, 1.0);
			camera.setPerspectiveProjection(glm::radians(50.0f), aspect, 0.1f, 10.0f);

			if (auto commandBuffer = sumiRenderer.beginFrame()) {

				int frameIdx = sumiRenderer.getFrameIdx();

				FrameInfo frameInfo{
					frameIdx,
					frameTime,
					commandBuffer,
					camera,
					globalDescriptorSets[frameIdx]
				};

				GlobalUBO ubo{};
				ubo.projectionView = camera.getProjectionMatrix() * camera.getViewMatrix();
				uniformBuffers[frameIdx]->writeToBuffer(&ubo);
				uniformBuffers[frameIdx]->flush();

				sumiRenderer.beginSwapChainRenderPass(commandBuffer);
				renderSystem.renderObjects(frameInfo, objects);
				sumiRenderer.endSwapChainRenderPass(commandBuffer);
				sumiRenderer.endFrame();
			}
		}

		// Prevent cleanup from happening while GPU resources are in use on close.
		vkDeviceWaitIdle(sumiDevice.device());
	}

	void Sumire::loadObjects() {
		std::shared_ptr<SumiModel> cubeModel = SumiModel::createFromFile(sumiDevice, "../models/clorinde.obj");
		auto renderObj = SumiObject::createObject();
		renderObj.model = cubeModel;
		renderObj.transform.translation = {0.0f, 0.0f, 0.0f};
		renderObj.transform.scale = 0.5f;

		objects.push_back(std::move(renderObj));
	}
}