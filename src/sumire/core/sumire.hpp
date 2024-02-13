#pragma once

#include <sumire/core/sumi_window.hpp>
#include <sumire/core/sumi_device.hpp>
#include <sumire/core/sumi_model.hpp>
#include <sumire/core/sumi_object.hpp>
#include <sumire/core/sumi_renderer.hpp>
#include <sumire/core/sumi_descriptors.hpp>

#include <memory>
#include <vector>

namespace sumire {

	class Sumire {
	public:
		static constexpr int WIDTH = 1280;
		static constexpr int HEIGHT = 720;

		void run();

		Sumire();
		~Sumire();

		Sumire(const Sumire&) = delete;
		Sumire& operator=(const Sumire&) = delete;

	private:

		void loadObjects();

		SumiWindow sumiWindow{ WIDTH, HEIGHT, "Sumire" };
		SumiDevice sumiDevice{ sumiWindow };
		SumiRenderer sumiRenderer{ sumiWindow, sumiDevice };

		std::unique_ptr<SumiDescriptorPool> globalDescriptorPool{};
		std::vector<SumiObject> objects;
	};
}