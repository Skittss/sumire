#pragma once

#include <string>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace sde {

	class SdeWindow {

	public:
		SdeWindow(int w, int h, std::string name);
		~SdeWindow();

		SdeWindow(const SdeWindow&) = delete;
		SdeWindow& operator=(const SdeWindow&) = delete;

		bool shouldClose() { return glfwWindowShouldClose(window); }
		VkExtent2D getExtent() { return { static_cast<uint32_t>(width), static_cast<uint32_t>(height) }; }
		bool wasWindowResized() { return fbResizeFlag; }
		void resetWindowResizedFlag() { fbResizeFlag = false; }

		void createWindowSurface(VkInstance instance, VkSurfaceKHR* surface);

	private:
		static void fbResizeCallback(GLFWwindow* window, int width, int height);
		void initWindow();

		int width;
		int height;
		bool fbResizeFlag = false;

		std::string windowName;

		GLFWwindow* window;

	};

}