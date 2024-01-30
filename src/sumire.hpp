#pragma once

#include "sumi_window.hpp"
#include "sumi_device.hpp"
#include "sumi_model.hpp"
#include "sumi_object.hpp"
#include "sumi_renderer.hpp"

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

		std::vector<SumiObject> objects;
	};
}