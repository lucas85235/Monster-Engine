#include <engine/InputHandler.h>
#include <engine/Log.h>

#include <iostream>

void InputHandler::initialize(GLFWwindow* window) {
    if (!window) {
        std::cerr << "InputHandler: Invalid window context\n";
        return;
    }

    setupMouseCapture(window);
    initializeMousePosition(window);
}

void InputHandler::processKeyboard(GLFWwindow* window, float deltaTime) {
    if (!camera_) return;

    const auto isPressed = [window](int key) { return glfwGetKey(window, key) == GLFW_PRESS; };

    camera_->processKeyboard(deltaTime,
                             isPressed(GLFW_KEY_W),              // forward
                             isPressed(GLFW_KEY_S),              // back
                             isPressed(GLFW_KEY_A),              // left
                             isPressed(GLFW_KEY_D),              // right
                             isPressed(GLFW_KEY_SPACE),          // up
                             isPressed(GLFW_KEY_LEFT_CONTROL));  // down
}

void InputHandler::processMousePosition(double xpos, double ypos) {
    if (!camera_) return;

    if (firstMouse_) {
        lastX_      = xpos;
        lastY_      = ypos;
        firstMouse_ = false;
        return;
    }

    double xOffset = xpos - lastX_;
    double yOffset = lastY_ - ypos;  // Y inverted in GLFW

    lastX_ = xpos;
    lastY_ = ypos;

    camera_->processMouseMovement(static_cast<float>(xOffset), static_cast<float>(yOffset));
}
void InputHandler::processMouseScroll(double xpos, double ypos) {
    if (!camera_) return;
    camera_->processMouseScroll(static_cast<float>(xpos), static_cast<float>(ypos));
}

void InputHandler::setCursorModeFromString(GLFWwindow* window, const std::string& modeString) {
    auto it = stringToCursorMode.find(modeString);

    if (it != stringToCursorMode.end()) {
        glfwSetInputMode(window, GLFW_CURSOR, static_cast<int>(it->second));
        SE_LOG_INFO("Cursor mode set to: {}", modeString);
    } else {
        SE_LOG_ERROR("Cursor mode not found: {}. The available ones are: normal, hidden, disabled", modeString);
    }
}

void InputHandler::setupMouseCapture(GLFWwindow* window) {
    glfwSetWindowUserPointer(window, this);

    // mouse position
    glfwSetCursorPosCallback(window, mousePositionCallback);

    // mouse scroll
    glfwSetScrollCallback(window, mouseScrollCallback);

    // mouse input mode
    setCursorModeFromString(window, "hidden");
}

void InputHandler::initializeMousePosition(GLFWwindow* window) {
    int width, height;
    glfwGetWindowSize(window, &width, &height);
    lastX_ = width / 2.0;
    lastY_ = height / 2.0;
}
