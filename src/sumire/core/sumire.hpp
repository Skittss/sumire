#pragma once

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
		static constexpr uint32_t STARTUP_WIDTH = 1920u;
		static constexpr uint32_t STARTUP_HEIGHT = 1080u;
		static constexpr uint32_t MAX_N_LIGHTS = 1024u;

		uint32_t screenWidth  = STARTUP_WIDTH;
		uint32_t screenHeight = STARTUP_HEIGHT;

		void run();

		Sumire();
		~Sumire();

		Sumire(const Sumire&) = delete;
		Sumire& operator=(const Sumire&) = delete;

	private:
		void loadObjects();
		void loadLights(); 

		SumiWindow sumiWindow{ 
			static_cast<int>(screenWidth), 
			static_cast<int>(screenHeight), 
			"Sumire" 
		};
		SumiDevice sumiDevice{ sumiWindow };
		SumiRenderer sumiRenderer{ sumiWindow, sumiDevice };

		std::unique_ptr<SumiDescriptorPool> globalDescriptorPool{};
		SumiObject::Map objects;
		SumiLight::Map lights;
	};
}