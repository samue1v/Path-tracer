#include "camera.hpp"
#include "Logger.hpp"

Camera::Camera(GLFWwindow *glfw_window, std::atomic<int> &_readIndex,
               std::atomic<int> &_writeIndex,
               std::array<Tracer::camera, 2> &_bufferRef,
               std::atomic<bool> &_cameraMoved)
    : readIndex(_readIndex), writeIndex(_writeIndex), cameraMoved(_cameraMoved),
      buffer_camera_(_bufferRef) {

  // Camera initial state.
  glfwGetWindowSize(glfw_window, &vp_size.x, &vp_size.y);
  resetView();
}

void Camera::resetView() {
  pos = glm::vec3(0., 0., 4.);
  yaw = -90.;
  pitch = 0.;
  right = glm::vec3(1.,0.,0.);
  update();
}

void Camera::walkFront() {
  pos += at * speed;// * deltaTime.load(std::memory_order::relaxed);
  update();
}
void Camera::walkBack() {
  pos -= at * speed;// * deltaTime.load(std::memory_order::relaxed);
  update();
}

void Camera::walkRight() {
  pos += right * speed;// * deltaTime.load(std::memory_order::relaxed);
  update();
}
void Camera::walkLeft() {
  pos -= right * speed;// * deltaTime.load(std::memory_order::relaxed);
  update();
}

void Camera::shake(float xoffset, float yoffset) {
  Logger::log(Logger::LogLevel::DEBUG, "Xoffset: " + std::to_string(xoffset) +", Yoffset: "+ std::to_string(yoffset));

  xoffset *= xsens;
  yoffset *= (ysens);

  yaw += xoffset;
  pitch += yoffset;

  if (pitch > 89.0f)
    pitch = 89.0f;
  if (pitch < -89.0f)
    pitch = -89.0f;

  update();
}

void Camera::update() {
  updateView();
  int wi = writeIndex.load(std::memory_order_relaxed);
  buffer_camera_[wi].view = camera_.view;
  buffer_camera_[wi].invView = glm::inverse(camera_.view);

  cameraMoved.store(1, std::memory_order_release);
  readIndex.store(wi, std::memory_order_release);
  writeIndex.store(1 - wi, std::memory_order_relaxed);
}

void Camera::updateView() {
  glm::vec3 front;
  front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
  front.y = sin(glm::radians(pitch));
  front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
  at = glm::normalize(front);

  right = glm::normalize(glm::cross(at, {0., 1., 0.}));
  up = glm::normalize(glm::cross(right, at));

  camera_.view = glm::lookAt(pos, pos + at, up);
}
