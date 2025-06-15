#ifndef GLFW_BACKEND
#define GLFW_BACKEND
#include <GLFW/glfw3.h>
#include <string>

class GLFW_backend {
public:
  static GLFW_backend &getInstance();
  void resizeWindow(int width, int height, const char *name);
  GLFWwindow *getWindow();

  static void framebufferResizeCallback(GLFWwindow *window, int width,
                                        int height);
  static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);

private:
  GLFW_backend();

private:
  GLFWwindow *window;
};

#endif
