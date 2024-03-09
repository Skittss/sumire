#pragma once

#include <string>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>

#include <unordered_map>

namespace sumire {

	class SumiWindow {

	public:
		SumiWindow(int w, int h, std::string name);
		~SumiWindow();

		SumiWindow(const SumiWindow&) = delete;
		SumiWindow& operator=(const SumiWindow&) = delete;

		bool shouldClose() { return glfwWindowShouldClose(window); }
		VkExtent2D getExtent() { return { static_cast<uint32_t>(width), static_cast<uint32_t>(height) }; }
		bool wasWindowResized() { return fbResizeFlag; }
		void resetWindowResizedFlag() { fbResizeFlag = false; }
		GLFWwindow *getGLFWwindow() const { return window; }

		void createWindowSurface(VkInstance instance, VkSurfaceKHR* surface);
		
		// Mouse Information
		glm::vec2 mouseDelta{0.0f};
		glm::vec2 mousePos{0.0f};
		glm::vec2 prevMousePos{width / 2.0f, height / 2.0f};
		void clearMouseDelta() { mouseDelta = glm::vec2{0.0f}; }

		// Keypress Information
		struct KeypressInfo {
			int scancode;
			int action;
			int mods;
		};
		using KeypressEvents = std::unordered_map<int, KeypressInfo>;
		KeypressEvents keypressEvents;
		void clearKeypressEvents() { keypressEvents.clear(); }

	private:
		static void fbResizeCallback(GLFWwindow* window, int width, int height);
		static void mousePosCallback(GLFWwindow* window, double xpos, double ypos);
		static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
		void initWindow();

		int width;
		int height;
		bool fbResizeFlag = false;

		std::string windowName;

		GLFWwindow* window;

	};

}