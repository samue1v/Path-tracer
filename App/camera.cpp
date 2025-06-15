#include "camera.hpp"

Camera::Camera(glm::vec3 _pos, glm::vec3 _at, float _aspect)
    : pos(_pos), at(_at), aspect(_aspect) {
  worldUp = glm::vec3(0., 1., 0.);
  up = worldUp;
  right = glm::vec3(1.);
  yaw = -90.;
  pitch = 0.;
  fov = 45.;
  near = 0.1;
  far = 100.;
  update();
}

void Camera::resetView() {

  pos = glm::vec3(0., 0., 4.);
  at = glm::vec3(0., 0., -1.);
  yaw = -90.;
  pitch = 0.;
  right = glm::vec3(1.);
  update();
}

void Camera::update() {
  updateView();
  updateProj();
}

void Camera::updateView() {
  // TODO
}

void Camera::updateProj() {
  vp.proj = glm::perspective(glm::radians(fov), aspect, near, far);
}
