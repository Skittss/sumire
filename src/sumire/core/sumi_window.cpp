#include <sumire/core/sumi_window.hpp>

#include <stdexcept>

namespace sumire {

	SumiWindow::SumiWindow(int w, int h, std::string name) : width(w), height(h), windowName(name) {
		initWindow();
	}

	SumiWindow::~SumiWindow() {
		glfwDestroyWindow(window);
		glfwTerminate();
	}

	void SumiWindow::initWindow() {
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

		window = glfwCreateWindow(width, height, windowName.c_str(), nullptr, nullptr);
		glfwSetWindowUserPointer(window, this);
		glfwSetFramebufferSizeCallback(window, fbResizeCallback);
	}

	void SumiWindow::createWindowSurface(VkInstance instance, VkSurfaceKHR* surface) {
		if (glfwCreateWindowSurface(instance, window, nullptr, surface) != VK_SUCCESS) {
			throw std::runtime_error("failed to create window surface");
		}
	}

	void SumiWindow::fbResizeCallback(GLFWwindow* window, int width, int height) {
		auto sumiWindow = reinterpret_cast<SumiWindow*>(glfwGetWindowUserPointer(window));
		sumiWindow->fbResizeFlag = true;
		sumiWindow->width = width;
		sumiWindow->height = height;
	}

}