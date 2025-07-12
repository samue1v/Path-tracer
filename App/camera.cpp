#include "camera.hpp"

Camera::Camera(std::atomic<int> &_readIndex, std::atomic<int> &_writeIndex,
               std::array<Tracer::camera, 2> &_bufferRef)
    : readIndex(_readIndex), writeIndex(_writeIndex), buffer(_bufferRef) {

  // Camera initial state.
  resetView();
}

void Camera::resetView() {

  pos = glm::vec3(0., 0., 4.);
  at = glm::vec3(0., 0., -1.);
  yaw = -90.;
  pitch = 0.;
  right = glm::vec3(1.);
  update();
}

void Camera::update() { updateView(); }

void Camera::updateView() {
  // TODO
