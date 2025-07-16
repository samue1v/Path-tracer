#include "app.hpp"

void spawn_render_thread(GLFWwindow *window, VulkanRender *engine,
                         std::atomic<bool> *done, std::atomic<int> *readIndex,
                         std::atomic<int> *writeIndex,
                         std::array<Tracer::camera, 2> *matrices) {
  engine->init();
  engine->app = static_cast<App *>(glfwGetWindowUserPointer(window));
  const int target_fps = 1;
  const long opt_time = target_fps > 0 ? 1'000'000'000 / target_fps : 0;

  long last_fps_time = 0;
  int fps = 0;

  auto last_time = std::chrono::high_resolution_clock::now();
  while (!*done) {
    auto now = std::chrono::high_resolution_clock::now();
    auto updateLength =
        duration_cast<std::chrono::nanoseconds>(now - last_time).count();
    last_time = now;
    last_fps_time += updateLength;
    fps++;

    if (last_fps_time >= 1'000'000'000) {
      engine->setCurrentFps(fps);
      last_fps_time = 0;
      fps = 0;
    }
    engine->drawFrame();
    if (target_fps > 0) {
      auto sleepTime =
          (opt_time - duration_cast<std::chrono::nanoseconds>(
                          std::chrono::high_resolution_clock::now() - last_time)
                          .count());
      if (sleepTime > 0)
        std::this_thread::sleep_for(std::chrono::nanoseconds(sleepTime));
    }
  }
  engine->waitIdle();

  delete engine;
}

App::App(GLFWwindow *window)
    : window(window), logger(Logger::getInstance()),
      camera(window, readIndex, writeIndex, camera_buffer) {

  engine = new VulkanRender(window);
  this->mainQueueMutex = &(engine->mainQueueMutex);
  this->mainCommands = &(engine->mainCommands);
}

void App::run() {
  if (enableValidationLayers) {
    Logger::log(Logger::LogLevel::INFO, "App is running!");
  }

  std::atomic<bool> done = false;
  glfwSetWindowUserPointer(window, this);
  std::thread render_thread(spawn_render_thread, window, engine, &done,
                            &readIndex, &writeIndex, &camera_buffer);

  main_loop();
  done = true;
  render_thread.join();
}

void App::main_loop() {
  int w, h;
  glfwGetWindowSize(window, &w, &h);
  glfwFocusWindow(window);
  glfwSetCursorPos(window, w / 2., h / 2.);
  glfwSetMouseButtonCallback(window, mouse_button_callback);
  glfwSetCursorPosCallback(window, cursor_position_callback);
  auto last_time = std::chrono::high_resolution_clock::now();
  const int target_update = 60;
  const long opt_time = target_update > 0 ? 1'000'000'000 / target_update : 0;
  long last_update_time = 0;

  while (!glfwWindowShouldClose(window)) {

    auto now = std::chrono::high_resolution_clock::now();
    auto updateLength =
        duration_cast<std::chrono::nanoseconds>(now - last_time).count();
    last_time = now;
    last_update_time += updateLength;

    if (last_update_time >= 1'000'000'000) {
      last_update_time = 0;
    }

    glfwPollEvents();
    updateState();
    processInput();

    if (target_update > 0) {
      auto sleepTime =
          (opt_time - duration_cast<std::chrono::nanoseconds>(
                          std::chrono::high_resolution_clock::now() - last_time)
                          .count()) /
          1'000'000;
      if (sleepTime > 0)
        std::this_thread::sleep_for(std::chrono::milliseconds(sleepTime));
    }
  }
}

void App::processInput() {}

void App::mouse_button_callback(GLFWwindow *window, int button, int action,
                                int mods) {
  auto app = static_cast<App *>(glfwGetWindowUserPointer(window));
}

void App::cursor_position_callback(GLFWwindow *window, double xpos,
                                   double ypos) {
  auto app = static_cast<App *>(glfwGetWindowUserPointer(window));
}

void App::uploadMesh(std::string name) {}

void App::updateState() {
  std::function<void()> command;
  {
    std::lock_guard<std::mutex> lock(*mainQueueMutex);
    if (!mainCommands->empty()) {
      command = mainCommands->front();
      mainCommands->pop();
    }
  }
  if (command)
    command();
}
