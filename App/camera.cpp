#include "camera.hpp"

Camera::Camera(GLFWwindow * glfw_window, std::atomic<int> &_readIndex,
               std::atomic<int> &_writeIndex,
               std::array<Tracer::camera, 2> &_bufferRef)
    :  readIndex(_readIndex), writeIndex(_writeIndex),
      buffer(_bufferRef) {

  // Camera initial state.
  glfwGetWindowSize(glfw_window,&vp_size.x,&vp_size.y);
  resetView();
}

void Camera::resetView() {

  yaw = -90.;
  pitch = 0.;
  right = glm::vec3(1.);

  float viewportWidth = vp_size.x;
  float fov_deg = 60.0f;
  float fov_rad = glm::radians(fov_deg);

  float vp_dist = (viewportWidth * 0.5f) / tan(fov_rad * 0.5f);

  pos = glm::vec3(0.0f, 0.0f, vp_dist);
  at = glm::vec3(0.f, 0.f, -1.f);
  up = glm::vec3(0.f, 1.f, 0.f);

  update();
}

void Camera::update() { updateView(); }

void Camera::updateView() {

  camera.pos = glm::vec4(pos, 1.f);
  camera.view = glm::lookAt(pos, at, up);
  camera.invView = glm::inverse(camera.view);
}
