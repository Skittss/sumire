#include "sumire.hpp"
#include "sumi_render_system.hpp"
#include "sumi_kbm_controller.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include <stdexcept>
#include <array>
#include <cassert>
#include <chrono>

namespace sumire {

	Sumire::Sumire() {
		loadObjects();
	}

	Sumire::~Sumire() {}

	void Sumire::run() {
		SumiRenderSystem renderSystem{sumiDevice, sumiRenderer.getSwapChainRenderPass()};
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
				sumiRenderer.beginSwapChainRenderPass(commandBuffer);
				renderSystem.renderObjects(commandBuffer, objects, camera);
				sumiRenderer.endSwapChainRenderPass(commandBuffer);
				sumiRenderer.endFrame();
			}
		}

		// Prevent cleanup from happening while GPU resources are in use on close.
		vkDeviceWaitIdle(sumiDevice.device());
	}

	void Sumire::loadObjects() {
		std::shared_ptr<SumiModel> cubeModel = SumiModel::createFromFile(sumiDevice, "../models/bunny.obj");
		auto renderObj = SumiObject::createObject();
		renderObj.model = cubeModel;
		renderObj.transform.translation = {0.0f, 0.0f, 0.0f};
		renderObj.transform.scale = {0.5f, 0.5f, 0.5f};

		objects.push_back(std::move(renderObj));
	}
}