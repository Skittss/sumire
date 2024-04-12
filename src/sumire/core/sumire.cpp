#include <sumire/core/sumire.hpp>
#include <sumire/core/graphics_pipeline/sumi_buffer.hpp>
#include <sumire/util/sumire_engine_path.hpp>

// Render systems
#include <sumire/core/render_systems/mesh_rendersys.hpp>
#include <sumire/core/render_systems/deferred_mesh_rendersys.hpp>
#include <sumire/core/render_systems/high_quality_shadow_mapper.hpp>
#include <sumire/core/render_systems/post_processor.hpp>
#include <sumire/core/render_systems/point_light_rendersys.hpp>
#include <sumire/core/render_systems/grid_rendersys.hpp>

// Asset loaders
#include <sumire/loaders/gltf_loader.hpp>
#include <sumire/loaders/obj_loader.hpp>

// input
#include <sumire/input/sumi_kbm_controller.hpp>

// GUI layer
#include <sumire/gui/sumi_imgui.hpp>

// glm
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtx/color_space.hpp>

#include <stdexcept>
#include <array>
#include <cassert>
#include <chrono>

namespace sumire {

	struct GlobalUBO {
		int nLights = 0;
	};

	struct CameraUBO {
		glm::mat4 projectionMatrix{1.0f};
		glm::mat4 viewMatrix{1.0f};
		glm::mat4 projectionViewMatrix{1.0f};
		glm::vec3 cameraPosition{0.0f};
	};

	Sumire::Sumire() {
		globalDescriptorPool = SumiDescriptorPool::Builder(sumiDevice)
			.setMaxSets(1 + (2 * SumiSwapChain::MAX_FRAMES_IN_FLIGHT))
			.addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, SumiSwapChain::MAX_FRAMES_IN_FLIGHT) // global
			.addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, SumiSwapChain::MAX_FRAMES_IN_FLIGHT) // camera
			.addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1) // Light SSBO
			.build();

		loadObjects();
		loadLights();
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

		// Light SSBO
		std::unique_ptr<SumiBuffer> lightSSBO = std::make_unique<SumiBuffer>(
			sumiDevice,
			MAX_N_LIGHTS * sizeof(SumiLight::LightShaderData),
			1,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
		);
		lightSSBO->map();

		// Layout
		auto globalDescriptorSetLayout = SumiDescriptorSetLayout::Builder(sumiDevice)
			.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)
			.addBinding(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)
			.addBinding(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)
			.build();

		// Write Descriptor Sets
		std::vector<VkDescriptorSet> globalDescriptorSets(SumiSwapChain::MAX_FRAMES_IN_FLIGHT);
		for (int i = 0; i < globalDescriptorSets.size(); i++) {
			auto globalBufferInfo = globalUniformBuffers[i]->descriptorInfo();
			auto cameraBufferInfo = cameraUniformBuffers[i]->descriptorInfo();
			auto lightSSBOinfo = lightSSBO->descriptorInfo(); // shared across swapchain images
			SumiDescriptorWriter(*globalDescriptorSetLayout, *globalDescriptorPool)
				.writeBuffer(0, &globalBufferInfo)
				.writeBuffer(1, &cameraBufferInfo)
				.writeBuffer(2, &lightSSBOinfo)
				.build(globalDescriptorSets[i]);
		}

		// Render Systems
		MeshRenderSys meshRenderSystem{
			sumiDevice,
			sumiRenderer.getRenderPass(), 
			sumiRenderer.forwardRenderSubpassIdx(),
			globalDescriptorSetLayout->getDescriptorSetLayout()
		};

		DeferredMeshRenderSys deferredMeshRenderSystem{
			sumiDevice,
			sumiRenderer.getGbuffer(),
			sumiRenderer.getRenderPass(), 
			sumiRenderer.gbufferFillSubpassIdx(),
			sumiRenderer.getRenderPass(),
			sumiRenderer.gbufferResolveSubpassIdx(),
			globalDescriptorSetLayout->getDescriptorSetLayout()
		};

		HighQualityShadowMapper shadowMapper{};

		PostProcessor postProcessor{
			sumiDevice,
			sumiRenderer.getIntermediateColorAttachments(),
			sumiRenderer.getCompositionRenderPass()
		};

		PointLightRenderSys pointLightSystem{
			sumiDevice, 
			sumiRenderer.getRenderPass(), 
			sumiRenderer.forwardRenderSubpassIdx(),
			globalDescriptorSetLayout->getDescriptorSetLayout()
		};

		GridRendersys gridRenderSystem{
			sumiDevice, 
			sumiRenderer.getRenderPass(),
			sumiRenderer.forwardRenderSubpassIdx(),
			globalDescriptorSetLayout->getDescriptorSetLayout()
		};

		sumiWindow.setMousePollMode(SumiWindow::MousePollMode::MANUAL);

		// Camera Control
		SumiCamera camera{glm::radians(50.0f), sumiRenderer.getAspect()};
		camera.transform.setTranslation(glm::vec3{0.0f, 1.0f, 3.0f});
		SumiKBMcontroller cameraController{
			sumiWindow,
			SumiKBMcontroller::ControllerType::FPS
		};

		// GUI
		SumiImgui gui{
			sumiRenderer,
			sumiRenderer.getCompositionRenderPass(),
			sumiRenderer.compositionSubpassIdx(),
			sumiDevice.presentQueue()
		};

		auto currentTime = std::chrono::high_resolution_clock::now();
		float cumulativeFrameTime = 0.0f;

		// Poll Mouse pos once directly before starting loop to prevent an incorrect
		//  Mouse update on start.
		sumiWindow.pollMousePos();

		// Draw loop
		while (!sumiWindow.shouldClose()) {
			sumiWindow.pollMousePos(); // if manual polling mouse pos
			// sumiWindow.clearMouseDelta(); // if using event-based polling
			sumiWindow.clearKeypressEvents();
			glfwPollEvents();

			// TODO:
			//  Prevent inputs from passthrough to application when ImGui wants to consume them...
			//  i.e. check io.WantCaptureMouse, etc.

			auto newTime = std::chrono::high_resolution_clock::now();
			float frameTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
			currentTime = newTime;

			const float MAX_FRAME_TIME = 0.2f; // 5fps
			frameTime = glm::min(frameTime, MAX_FRAME_TIME);

			cumulativeFrameTime += frameTime;

			// Handle input.
			// TODO: move components to member variables and call this from a function
			auto guiIO = gui.getIO();

			if (sumiWindow.isCursorHidden()) gui.ignoreMouse();
			else gui.enableMouse();

			if (!(guiIO.WantCaptureKeyboard && guiIO.WantTextInput)) {

				glm::vec2 mouseDelta = (!sumiWindow.isCursorHidden() && guiIO.WantCaptureMouse)
					? glm::tvec2<double>{0.0f}
					: sumiWindow.mouseDelta;

				cameraController.move(
					frameTime, 
					sumiWindow.keypressEvents,
					mouseDelta,
					camera.getOrthonormalBasis(),
					camera.transform
				);
			}

			camera.setViewYXZ(camera.transform.getTranslation(), camera.transform.getRotation());

			if (sumiRenderer.wasSwapChainRecreated()) {
				float aspect = sumiRenderer.getAspect();
				camera.setAspect(aspect, true);
				postProcessor.updateDescriptors(sumiRenderer.getIntermediateColorAttachments());
				sumiRenderer.resetScRecreatedFlag();
			}

			if (sumiRenderer.wasGbufferRecreated()) {
				deferredMeshRenderSystem.updateResolveDescriptors(sumiRenderer.getGbuffer());
				sumiRenderer.resetGbufferRecreatedFlag();
			}

			// Set up FrameInfo for GUI, and add remaining props later;
			FrameInfo frameInfo{
				-1,        // frameIdx
				frameTime,
				cumulativeFrameTime,
				nullptr,   // commandBuffer
				camera,
				nullptr,   // globalDescriptorSet
				objects,
				lights
			};

			// Render GUI
			gui.beginFrame();
			gui.drawStatWindow(frameInfo, cameraController);
			gui.endFrame();

			SumiRenderer::FrameCommandBuffers frameCommandBuffers = sumiRenderer.beginFrame();

			// If all cmd buffers are null, then the frame was not started.
			if (frameCommandBuffers.validFrame()) {

				int frameIdx = sumiRenderer.getFrameIdx();

				// Fill in rest of frame-specific frameinfo props
				frameInfo.frameIdx = frameIdx;
				frameInfo.commandBuffer = frameCommandBuffers.graphics;
				frameInfo.globalDescriptorSet = globalDescriptorSets[frameIdx];

				// Populate uniform buffers with data
				CameraUBO cameraUbo{};
				cameraUbo.projectionMatrix     = camera.getProjectionMatrix();
				cameraUbo.viewMatrix           = camera.getViewMatrix();
				cameraUbo.projectionViewMatrix = cameraUbo.projectionMatrix * cameraUbo.viewMatrix;
				cameraUbo.cameraPosition = camera.transform.getTranslation();
				cameraUniformBuffers[frameIdx]->writeToBuffer(&cameraUbo);
				cameraUniformBuffers[frameIdx]->flush();

				const int nLights = static_cast<int>(lights.size());
				GlobalUBO globalUbo{};
				globalUbo.nLights = nLights;
				globalUniformBuffers[frameIdx]->writeToBuffer(&globalUbo);
				globalUniformBuffers[frameIdx]->flush();

				// Write lights SSBO
				// TODO: This will be very slow when the number of lights increases.
				//        We should only write to the buffer when absolutely necessary, i.e. on light change.
				auto lightData = std::vector<SumiLight::LightShaderData>{};
				for (auto& kv : lights) {
					auto& light = kv.second;
					lightData.push_back(light.getShaderData());
				}
				lightSSBO->writeToBuffer(lightData.data(), nLights * sizeof(SumiLight::LightShaderData));
				lightSSBO->flush();

				// Shadow mapping
				shadowMapper.prepare(
					lightData,
					camera.getNear(), camera.getFar(), camera.getFovy(),
					cameraUbo.viewMatrix
				);

				// Main scene render pass
				frameInfo.commandBuffer = frameCommandBuffers.graphics;
				sumiRenderer.beginRenderPass(frameCommandBuffers.graphics);

				// Deferred fill subpass
				deferredMeshRenderSystem.fillGbuffer(frameInfo);

				sumiRenderer.nextSubpass(frameCommandBuffers.graphics);

				// Deferred resolve subpass
				deferredMeshRenderSystem.resolveGbuffer(frameInfo);

				sumiRenderer.nextSubpass(frameCommandBuffers.graphics);

				// Forward rendering subpass
				pointLightSystem.render(frameInfo);
				
				if (gui.showGrid && gui.gridOpacity > 0.0f) {
					auto gridUbo = gui.getGridUboData();
					gridRenderSystem.render(frameInfo, gridUbo);
				}
				
				sumiRenderer.endRenderPass(frameCommandBuffers.graphics);

				// Async Compute for post effects
				sumiRenderer.beginPostCompute(frameCommandBuffers.compute);

				postProcessor.tonemap(frameCommandBuffers.compute, frameInfo.frameIdx);

				sumiRenderer.endPostCompute(frameCommandBuffers.compute);

				// Final Composite
				sumiRenderer.beginCompositeRenderPass(frameCommandBuffers.present);

				postProcessor.compositeFrame(frameCommandBuffers.present, frameInfo.frameIdx);

				// GUI should *ALWAYS* render last after post-processing.
				gui.renderToCmdBuffer(frameCommandBuffers.present);

				sumiRenderer.endCompositeRenderPass(frameCommandBuffers.present);

				sumiRenderer.endFrame();
			}
		}

		// Prevent cleanup from happening while GPU resources are in use on close.
		//  TODO: can this error?
		vkDeviceWaitIdle(sumiDevice.device());
	}

	void Sumire::loadObjects() {
		// TODO: Load objects in asynchronously
		// std::shared_ptr<SumiModel> modelObj1 = loaders::OBJloader::createModelFromFile(sumiDevice, SUMIRE_ENGINE_PATH("assets/models/obj/clorinde.obj"));
		//std::shared_ptr<SumiModel> modelObj1 = loaders::GLTFloader::createModelFromFile(sumiDevice, SUMIRE_ENGINE_PATH("assets/models/gltf/test/NormalTangentMirrorTest.glb"));
		std::shared_ptr<SumiModel> modelObj1 = loaders::GLTFloader::createModelFromFile(sumiDevice, SUMIRE_ENGINE_PATH("assets/models/gltf/clorinde.glb"));
		auto obj1 = SumiObject::createObject();
		obj1.model = modelObj1;
		obj1.transform.setTranslation(glm::vec3{-8.0f, 0.0f, 0.0f});
		obj1.transform.setScale(glm::vec3{1.0f});
		objects.emplace(obj1.getId(), std::move(obj1));

		std::shared_ptr<SumiModel> modelGlb1 = loaders::GLTFloader::createModelFromFile(sumiDevice, SUMIRE_ENGINE_PATH("assets/models/gltf/test/MetalRoughSpheres.glb"));
		auto glb1 = SumiObject::createObject();
		glb1.model = modelGlb1;
		glb1.transform.setTranslation(glm::vec3{-4.0f, 0.0f, 0.0f});
		glb1.transform.setScale(glm::vec3{0.2f});
		objects.emplace(glb1.getId(), std::move(glb1));

		// std::shared_ptr<SumiModel> modelGlb2 = loaders::GLTFloader::createModelFromFile(sumiDevice, SUMIRE_ENGINE_PATH("assets/models/gltf/doomslayer.glb"));
		// auto glb2 = SumiObject::createObject();
		// glb2.model = modelGlb2;
		// glb2.transform.setTranslation(glm::vec3{0.0f, 0.0f, 0.0f});
		// glb2.transform.setScale(glm::vec3{1.0f});
		// objects.emplace(glb2.getId(), std::move(glb2));

		std::shared_ptr<SumiModel> modelGlb3 = loaders::GLTFloader::createModelFromFile(sumiDevice, SUMIRE_ENGINE_PATH("assets/models/gltf/2b.glb"));
		auto glb3 = SumiObject::createObject();
		glb3.model = modelGlb3;
		glb3.transform.setTranslation(glm::vec3{0.0f, 0.0f, 0.0f});
		glb3.transform.setScale(glm::vec3{1.0f});
		objects.emplace(glb3.getId(), std::move(glb3));
	}

	void Sumire::loadLights() {
		constexpr float radial_n_lights = 20.0;
		for (float i = 0; i < radial_n_lights; i++) {
			float rads = i * glm::two_pi<float>() / radial_n_lights;
			auto light = SumiLight::createPointLight(glm::vec3{1.5f * glm::sin(rads), 3.0f, 1.5f * glm::cos(rads)});
			float hue = i * 360.0f / radial_n_lights;
			light.color = glm::vec4(glm::rgbColor(glm::vec3{ hue, 1.0, 1.0 }), 1.0);
			lights.emplace(light.getId(), std::move(light));
		}

	}
}