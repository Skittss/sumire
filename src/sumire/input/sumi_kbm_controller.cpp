#include <sumire/input/sumi_kbm_controller.hpp>

namespace sumire {

    void SumiKBMcontroller::moveWalk(GLFWwindow* window, float dt, Transform3DComponent &transform) {

        glm::vec3 rotate{0};
        if (glfwGetKey(window, keybinds.lookRight) == GLFW_PRESS) rotate.y += 1.0f;
        if (glfwGetKey(window, keybinds.lookLeft ) == GLFW_PRESS) rotate.y -= 1.0f;
        if (glfwGetKey(window, keybinds.lookUp   ) == GLFW_PRESS) rotate.x += 1.0f;
        if (glfwGetKey(window, keybinds.lookDown ) == GLFW_PRESS) rotate.x -= 1.0f;

        glm::vec3 newRotation = transform.getRotation();
        if (glm::dot(rotate, rotate) > std::numeric_limits<float>::epsilon()) {
            newRotation += lookSensitivity * dt * glm::normalize(rotate);
        }

        newRotation.x = glm::clamp(newRotation.x, -1.5f, 1.5f);
        newRotation.y = glm::mod(newRotation.y, glm::two_pi<float>());

        transform.setRotation(newRotation);

        float yaw = newRotation.y;
        const glm::vec3 forward{sin(yaw), 0.0f, cos(yaw)};
        const glm::vec3 right{forward.z, 0.0f, -forward.x};
        const glm::vec3 up{0.0f, -1.0f, 0.0f}; 

        glm::vec3 moveDir{0.0f};
        if (glfwGetKey(window, keybinds.moveForward ) == GLFW_PRESS) moveDir += forward;
        if (glfwGetKey(window, keybinds.moveBackward) == GLFW_PRESS) moveDir -= forward;
        if (glfwGetKey(window, keybinds.moveRight   ) == GLFW_PRESS) moveDir += right;
        if (glfwGetKey(window, keybinds.moveLeft    ) == GLFW_PRESS) moveDir -= right;
        if (glfwGetKey(window, keybinds.moveUp      ) == GLFW_PRESS) moveDir += up;
        if (glfwGetKey(window, keybinds.moveDown    ) == GLFW_PRESS) moveDir -= up;

        if (glm::dot(moveDir, moveDir) > std::numeric_limits<float>::epsilon()) {
            transform.setTranslation(transform.getTranslation() + moveSensitivity * dt * glm::normalize(moveDir));
        }

    }

}