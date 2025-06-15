#ifndef APP_HPP
#define APP_HPP
#include "config.hpp"
#include "glfw_backend.hpp"
#include "vulkan_renderer.hpp"
#include <GLFW/glfw3.h>
#include <array>
#include <atomic>
#include <iostream>
#include <mutex>
#include <thread>


/**
 * @brief The main program.
 */
class App {
public:
  /**
   * @brief Construct a new app
   *
   * @param window The main window for the program
   */
  App(GLFWwindow *window);

  /**
   * @brief Run program
   */
  void run();

  /**
   * @brief Upload new Mesh
   * @param name mesh name
   */
  void uploadMesh(std::string name);
                 

private:
  /**
   * @brief run the program
   */
  void main_loop();

  /**
   * @brief Process window input
   */
  void processInput();

  /**
   * @brief Mouse callback
   */
  static void mouse_button_callback(GLFWwindow *window, int button, int action,
                                    int mods);

  /**
   * @brief Cursor movement callback
   */
  static void cursor_position_callback(GLFWwindow *window, double xpos,
                                       double ypos);

  void updateState();

private:
  /**
   * @brief the main window for the program
   */
  GLFWwindow *window;

  std::queue<std::function<void()>> renderCommands;
  std::mutex renderQueueMutex;


  std::queue<std::function<void()>> * mainCommands;
  std::mutex * mainQueueMutex;

public:

  VulkanRender *engine;
};

#endif
