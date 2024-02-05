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

	std::unique_ptr<SumiModel> createCubeModel(SumiDevice& device, glm::vec3 offset) {
		SumiModel::Data modelData{};
		modelData.vertices = {
			// left face (white)
			{{-.5f, -.5f, -.5f}, {.9f, .9f, .9f}},
			{{-.5f, .5f, .5f}, {.9f, .9f, .9f}},
			{{-.5f, -.5f, .5f}, {.9f, .9f, .9f}},
			{{-.5f, .5f, -.5f}, {.9f, .9f, .9f}},

			// right face (yellow)
			{{.5f, -.5f, -.5f}, {.8f, .8f, .1f}},
			{{.5f, .5f, .5f}, {.8f, .8f, .1f}},
			{{.5f, -.5f, .5f}, {.8f, .8f, .1f}},
			{{.5f, .5f, -.5f}, {.8f, .8f, .1f}},

			// top face (orange, remember y axis points down)
			{{-.5f, -.5f, -.5f}, {.9f, .6f, .1f}},
			{{.5f, -.5f, .5f}, {.9f, .6f, .1f}},
			{{-.5f, -.5f, .5f}, {.9f, .6f, .1f}},
			{{.5f, -.5f, -.5f}, {.9f, .6f, .1f}},

			// bottom face (red)
			{{-.5f, .5f, -.5f}, {.8f, .1f, .1f}},
			{{.5f, .5f, .5f}, {.8f, .1f, .1f}},
			{{-.5f, .5f, .5f}, {.8f, .1f, .1f}},
			{{.5f, .5f, -.5f}, {.8f, .1f, .1f}},

			// nose face (blue)
			{{-.5f, -.5f, 0.5f}, {.1f, .1f, .8f}},
			{{.5f, .5f, 0.5f}, {.1f, .1f, .8f}},
			{{-.5f, .5f, 0.5f}, {.1f, .1f, .8f}},
			{{.5f, -.5f, 0.5f}, {.1f, .1f, .8f}},

			// tail face (green)
			{{-.5f, -.5f, -0.5f}, {.1f, .8f, .1f}},
			{{.5f, .5f, -0.5f}, {.1f, .8f, .1f}},
			{{-.5f, .5f, -0.5f}, {.1f, .8f, .1f}},
			{{.5f, -.5f, -0.5f}, {.1f, .8f, .1f}},
		};

		for (auto& v : modelData.vertices) {
			v.position += offset;
		}

		modelData.indices = {0,  1,  2,  0,  3,  1,  4,  5,  6,  4,  7,  5,  8,  9,  10, 8,  11, 9,
							 12, 13, 14, 12, 15, 13, 16, 17, 18, 16, 19, 17, 20, 21, 22, 20, 23, 21};

		return std::make_unique<SumiModel>(device, modelData);
	}

	void Sumire::loadObjects() {
		std::shared_ptr<SumiModel> cubeModel = createCubeModel(sumiDevice, {0.0f, 0.0f, 0.0f});
		auto cube = SumiObject::createObject();
		cube.model = cubeModel;
		cube.transform.translation = {0.0f, 0.0f, 0.0f};
		cube.transform.scale = {0.5f, 0.5f, 0.5f};

		objects.push_back(std::move(cube));
	}
}