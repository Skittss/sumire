#include <sumire/core/sumire.hpp>
#include <sumire/core/sumi_buffer.hpp>

// Render systems
#include <sumire/core/render_systems/mesh_rendersys.hpp>
#include <sumire/core/render_systems/point_light_rendersys.hpp>
#include <sumire/core/render_systems/grid_rendersys.hpp>

// input
#include <sumire/input/sumi_kbm_controller.hpp>

// GUI layer
#include <sumire/gui/sumi_imgui.hpp>

// glm
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include <stdexcept>
#include <array>
#include <cassert>
#include <chrono>

namespace sumire {

	struct GlobalUBO {
		alignas(16) glm::vec3 ambientCol{0.02f};
		alignas(16) glm::vec3 lightDir = glm::normalize(glm::vec3{1.0f, 1.0f, 1.0f});
		alignas(16) glm::vec3 lightPos{-1.0f};
		alignas(16) glm::vec3 lightCol{ 1.0f};
		alignas(4)  float lightIntesnity = 1.0f;
	};
	
	struct CameraUBO {
		alignas(16) glm::mat4 projectionMatrix{1.0f};
		alignas(16) glm::mat4 viewMatrix{1.0f};
		alignas(16) glm::mat4 projectionViewMatrix{1.0f};
	};

	struct DirectionalLightUBO {
		alignas(16) glm::vec3 lightDir = glm::normalize(glm::vec3{1.0f, 1.0f, 1.0f});
		alignas(16) glm::vec3 lightCol{ 1.0f};
		alignas(4)  float lightIntesnity = 1.0f;
	};

	struct PointLightUBO {
		alignas(16) glm::vec3 lightPos{-1.0f};
		alignas(16) glm::vec3 lightCol{ 1.0f};
		alignas(4)  float lightIntesnity = 1.0f;
	};

	Sumire::Sumire() {
		globalDescriptorPool = SumiDescriptorPool::Builder(sumiDevice)
			.setMaxSets(SumiSwapChain::MAX_FRAMES_IN_FLIGHT)
			.addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, SumiSwapChain::MAX_FRAMES_IN_FLIGHT) // global
			.addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, SumiSwapChain::MAX_FRAMES_IN_FLIGHT) // camera
			.build();

		loadObjects();
	}

	Sumire::~Sumire() {
		globalDescriptorPool = nullptr; // Clean up pools before device etc are cleaned up.
	}

	void Sumire::run() {

		// --------------------- GLOBAL DESCRIPTORS (SET 0)
		// Uniform Buffers
		std::vector<std::unique_ptr<SumiBuffer>> globalUniformBuffers(SumiSwapChain::MAX_FRAMES_IN_FLIGHT);
		std::vector<std::unique_ptr<SumiBuffer>> cameraUniformBuffers(SumiSwapChain::MAX_FRAMES_IN_FLIGHT);
		for (int i = 0; i < SumiSwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
			
			// Global uniforms
			globalUniformBuffers[i] = std::make_unique<SumiBuffer>(
				sumiDevice,
				sizeof(GlobalUBO),
				1,
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
			);
			globalUniformBuffers[i]->map(); //enable writing to buffer memory

			// Camera uniforms
			cameraUniformBuffers[i] = std::make_unique<SumiBuffer>(
				sumiDevice,
				sizeof(CameraUBO),
				1,
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
			);
			cameraUniformBuffers[i]->map(); //enable writing to buffer memory
		}

		// Layout
		auto globalDescriptorSetLayout = SumiDescriptorSetLayout::Builder(sumiDevice)
			.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)
			.addBinding(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)
			.build();

		// 
		std::vector<VkDescriptorSet> globalDescriptorSets(SumiSwapChain::MAX_FRAMES_IN_FLIGHT);
		for (int i = 0; i < globalDescriptorSets.size(); i++) {
			auto globalBufferInfo = globalUniformBuffers[i]->descriptorInfo();
			auto cameraBufferInfo = cameraUniformBuffers[i]->descriptorInfo();
			SumiDescriptorWriter(*globalDescriptorSetLayout, *globalDescriptorPool)
				.writeBuffer(0, &globalBufferInfo)
				.writeBuffer(1, &cameraBufferInfo)
				.build(globalDescriptorSets[i]);
		}

		MeshRenderSys renderSystem{
			sumiDevice, sumiRenderer.getSwapChainRenderPass(), globalDescriptorSetLayout->getDescriptorSetLayout()};

		PointLightRenderSys pointLightSystem{
			sumiDevice, sumiRenderer.getSwapChainRenderPass(), globalDescriptorSetLayout->getDescriptorSetLayout()};

		GridRendersys gridRenderSystem{
			sumiDevice, sumiRenderer.getSwapChainRenderPass(), globalDescriptorSetLayout->getDescriptorSetLayout()};

		SumiCamera camera{glm::radians(50.0f), sumiRenderer.getAspect()};
		camera.transform.translation = {0.0f, -0.5f, -3.0f};
		//camera.setViewTarget(glm::vec3(2.0f), glm::vec3(0.0f));
		SumiKBMcontroller cameraController{};

		// GUI
		SumiImgui gui{sumiRenderer};

		auto currentTime = std::chrono::high_resolution_clock::now();

		// Draw loop
		while (!sumiWindow.shouldClose()) {
			glfwPollEvents();
			// TODO:
			//  Prevent inputs from passthrough to application when ImGui wants to consume them...
			//  i.e. check io.WantCaptureMouse, etc.

			auto newTime = std::chrono::high_resolution_clock::now();
			float frameTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
			currentTime = newTime;

			const float MAX_FRAME_TIME = 0.2f; // 5fps
			frameTime = glm::min(frameTime, MAX_FRAME_TIME);

			// Handle input.
			// TODO: move components to member variables and call this from a function
			auto guiIO = gui.getIO();
			if (!(guiIO.WantCaptureKeyboard && guiIO.WantTextInput)) {
				cameraController.moveWalk(sumiWindow.getGLFWwindow(), frameTime, camera.transform);
			}

			camera.setViewYXZ(camera.transform.translation, camera.transform.rotation);

			if (sumiRenderer.wasSwapChainRecreated()) {
				float aspect = sumiRenderer.getAspect();
				camera.setAspect(aspect, true);
				sumiRenderer.resetScRecreatedFlag();
			}
			//camera.setOrthographicProjection(-aspect, aspect, -1.0, 1.0, -1.0, 1.0);
			//camera.setPerspectiveProjection(glm::radians(50.0f), aspect);

			// Set up FrameInfo for GUI, and add remaining props later;
			FrameInfo frameInfo{
				-1,        // frameIdx
				frameTime,
				nullptr,   // commandBuffer
				camera,
				nullptr,   // globalDescriptorSet
				objects
			};

			// Render GUI
			gui.beginFrame();
			gui.drawStatWindow(frameInfo);
			gui.endFrame();

			if (auto commandBuffer = sumiRenderer.beginFrame()) {

				int frameIdx = sumiRenderer.getFrameIdx();

				// Fill in rest of frame-specific frameinfo props
				frameInfo.frameIdx = frameIdx;
				frameInfo.commandBuffer = commandBuffer;
				frameInfo.globalDescriptorSet = globalDescriptorSets[frameIdx];

				// Populate uniform buffers with data
				CameraUBO cameraUbo{};
				cameraUbo.projectionMatrix     = camera.getProjectionMatrix();
				cameraUbo.viewMatrix           = camera.getViewMatrix();
				cameraUbo.projectionViewMatrix = cameraUbo.projectionMatrix * cameraUbo.viewMatrix;
				cameraUniformBuffers[frameIdx]->writeToBuffer(&cameraUbo);
				cameraUniformBuffers[frameIdx]->flush();

				GlobalUBO globalUbo{};
				globalUniformBuffers[frameIdx]->writeToBuffer(&globalUbo);
				globalUniformBuffers[frameIdx]->flush();

				sumiRenderer.beginSwapChainRenderPass(commandBuffer);

				renderSystem.renderObjects(frameInfo);
				pointLightSystem.render(frameInfo);
				gridRenderSystem.render(frameInfo);

				// GUI should *ALWAYS* render last.
				gui.renderToCmdBuffer(commandBuffer);
				
				sumiRenderer.endSwapChainRenderPass(commandBuffer);

				sumiRenderer.endFrame();
			}
		}

		// Prevent cleanup from happening while GPU resources are in use on close.
		//  TODO: can this error?
		vkDeviceWaitIdle(sumiDevice.device());
	}

	void Sumire::loadObjects() {
		// TODO: Obselete; remove as replaced by grid rendersys.
		std::shared_ptr<SumiModel> quad = SumiModel::createFromFile(sumiDevice, "../models/primitives/quad.obj");
		auto grid = SumiObject::createObject();
		grid.model = quad;
		grid.transform.translation = {0.0f, 0.0f, 0.0f};
		grid.transform.scale = 1.0f;
		//objects.emplace(grid.getId(), std::move(grid));

		std::shared_ptr<SumiModel> cubeModel = SumiModel::createFromFile(sumiDevice, "../models/clorinde.obj");
		auto renderObj = SumiObject::createObject();
		renderObj.model = cubeModel;
		renderObj.transform.translation = {0.0f, 0.0f, 0.0f};
		renderObj.transform.scale = 0.5f;
		objects.emplace(renderObj.getId(), std::move(renderObj));
	}
}