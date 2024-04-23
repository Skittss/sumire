#pragma once

#include <sumire/config/sumi_config.hpp>

#include <sumire/core/windowing/sumi_window.hpp>
#include <sumire/core/graphics_pipeline/sumi_device.hpp>
#include <sumire/core/graphics_pipeline/sumi_descriptors.hpp>
#include <sumire/core/models/sumi_model.hpp>
#include <sumire/core/rendering/sumi_object.hpp>
#include <sumire/core/rendering/sumi_light.hpp>
#include <sumire/core/rendering/sumi_renderer.hpp>

#include <memory>
#include <vector>

namespace sumire {

	class Sumire {
	public:
		Sumire();
		~Sumire() = default;

		Sumire(const Sumire&) = delete;
		Sumire& operator=(const Sumire&) = delete;

		uint32_t screenWidth = 0;
		uint32_t screenHeight = 0;

		void init();
		void run();

	private:
		void loadObjects();
		void loadLights(); 

		SumiConfig sumiConfig{};
		SumiWindow sumiWindow{ 
			static_cast<int>(sumiConfig.configData.RESOLUTION.WIDTH),
			static_cast<int>(sumiConfig.configData.RESOLUTION.HEIGHT),
			"Sumire" 
		};
		SumiDevice sumiDevice{ sumiWindow, &sumiConfig };
		SumiRenderer sumiRenderer{ sumiWindow, sumiDevice, sumiConfig };

		// Global descriptors and buffers
		std::unique_ptr<SumiDescriptorPool> globalDescriptorPool;
		std::unique_ptr<SumiDescriptorSetLayout> globalDescriptorSetLayout;
		std::vector<VkDescriptorSet> globalDescriptorSets{};

		std::vector<std::unique_ptr<SumiBuffer>> globalUniformBuffers;
		std::vector<std::unique_ptr<SumiBuffer>> cameraUniformBuffers;
		std::unique_ptr<SumiBuffer> lightSSBO;

		SumiObject::Map objects;
		SumiLight::Map lights;
	};
}