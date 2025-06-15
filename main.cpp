#include "../App/app.hpp"
#include <GLFW/glfw3.h>
#include <atomic>
#include <thread>


int main() {
  GLFW_backend back = GLFW_backend::getInstance();
  GLFWwindow *window = back.getWindow();

  App *app = new App(window);
  app->run();
  glfwTerminate();
  delete app;
  return 0;
}
