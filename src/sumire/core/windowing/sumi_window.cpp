#include <sumire/core/windowing/sumi_window.hpp>

#include <stdexcept>
#include <iostream>

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

		setMousePollMode(MousePollMode::MANUAL);
		if (glfwRawMouseMotionSupported)
			glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);

		glfwSetKeyCallback(window, keyCallback);
	}

	void SumiWindow::createWindowSurface(VkInstance instance, VkSurfaceKHR* surface) {
		if (glfwCreateWindowSurface(instance, window, nullptr, surface) != VK_SUCCESS) {
			throw std::runtime_error("failed to create window surface");
		}
	}

	void SumiWindow::showCursor() {
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		cursorHidden = false;
	}

	void SumiWindow::disableCursor() {
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		cursorHidden = true;
	}

	void SumiWindow::fbResizeCallback(GLFWwindow* window, int width, int height) {
		auto sumiWindow = reinterpret_cast<SumiWindow*>(glfwGetWindowUserPointer(window));
		sumiWindow->fbResizeFlag = true;
		sumiWindow->width = width;
		sumiWindow->height = height;
	}

	void SumiWindow::setMousePollMode(MousePollMode pollMode) {
		mousePollMode = pollMode;
		switch(pollMode) {
			case MousePollMode::EVENT_BASED: {
				glfwSetCursorPosCallback(window, mousePosCallback);
			}
			break;
			case MousePollMode::MANUAL:
			default: {
				glfwSetCursorPosCallback(window, nullptr);
			}
		}
	}

	void SumiWindow::pollMousePos() {
		prevMousePos = mousePos;
		getMousePos(mousePos);
		mouseDelta = mousePos - prevMousePos;
	}

	void SumiWindow::mousePosCallback(GLFWwindow* window, double xpos, double ypos) {
		auto sumiWindow = reinterpret_cast<SumiWindow*>(glfwGetWindowUserPointer(window));
		sumiWindow->prevMousePos = sumiWindow->mousePos;
		sumiWindow->mousePos = glm::tvec2<double>{xpos, ypos};
		sumiWindow->mouseDelta = sumiWindow->mousePos - sumiWindow->prevMousePos;
	}

	void SumiWindow::keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods) {
		// Add callback keypress and its info to keypress event map
		auto sumiWindow = reinterpret_cast<SumiWindow*>(glfwGetWindowUserPointer(window));
		sumiWindow->keypressEvents.emplace(key, KeypressInfo{scancode, action, mods});
	}
}