#include "sde_window.hpp"

#include <stdexcept>

namespace sde {

	SdeWindow::SdeWindow(int w, int h, std::string name) : width(w), height(h), windowName(name) {
		initWindow();
	}

	SdeWindow::~SdeWindow() {
		glfwDestroyWindow(window);
		glfwTerminate();
	}

	void SdeWindow::initWindow() {
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

		window = glfwCreateWindow(width, height, windowName.c_str(), nullptr, nullptr);
		glfwSetWindowUserPointer(window, this);
		glfwSetFramebufferSizeCallback(window, fbResizeCallback);
	}

	void SdeWindow::createWindowSurface(VkInstance instance, VkSurfaceKHR* surface) {
		if (glfwCreateWindowSurface(instance, window, nullptr, surface) != VK_SUCCESS) {
			throw std::runtime_error("failed to create window surface");
		}
	}

	void SdeWindow::fbResizeCallback(GLFWwindow* window, int width, int height) {
		auto sdeWindow = reinterpret_cast<SdeWindow*>(glfwGetWindowUserPointer(window));
		sdeWindow->fbResizeFlag = true;
		sdeWindow->width = width;
		sdeWindow->height = height;
	}

}