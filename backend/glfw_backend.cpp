#include "glfw_backend.hpp"
#include <GLFW/glfw3.h>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include "app.hpp"
GLFW_backend::GLFW_backend() {
  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_FLOATING, GLFW_TRUE);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
  window = glfwCreateWindow(640, 640, "Vulkan", nullptr, nullptr);
  glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
  glfwSwapInterval(0);

  if (!window) {
    throw std::runtime_error("Could not create GLFW Window.");
  }
}

GLFW_backend &GLFW_backend::getInstance() {
  static GLFW_backend instance;
  return instance;
}

void GLFW_backend::resizeWindow(int width, int height, const char *name) {}

GLFWwindow *GLFW_backend::getWindow() { return this->window; }

void GLFW_backend::framebufferResizeCallback(GLFWwindow *window, int width,
                                             int height) {
  if (glfwGetWindowUserPointer(window) == nullptr) {
    return;
  }
}

void GLFW_backend::key_callback(GLFWwindow *window, int key, int scancode, int action,
                  int mods) {

  auto app = reinterpret_cast<App *>(glfwGetWindowUserPointer(window));
  if (key == GLFW_KEY_D && action == GLFW_PRESS) {
  }

  if (key == GLFW_KEY_A && action == GLFW_PRESS) {
  }
  glfwPollEvents();
}
