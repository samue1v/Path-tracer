#ifndef CAMERA_HPP
#define CAMERA_HPP

#include "shared_structs.hpp"
#include <array>
#include <atomic>
#include <chrono>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_projection.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/glm.hpp>
#include <glm/matrix.hpp>
#include <glm/vec4.hpp>

static float speed = .1f;
static float xsens = .001f;
static float ysens = .001f;

class Camera {
public:
  Camera(glm::vec3 pos, glm::vec3 at, float aspect);

  void resetView();

  void updateView();
  void updateProj();

  void update();

public:
  Shared::CameraMatrices vp;

  glm::vec3 at;
  glm::vec3 pos;
  glm::vec3 right;
  glm::vec3 up;
  glm::vec3 worldUp;
  float yaw;
  float pitch;

  float fov;
  float near;
  float far;
  float aspect;
};

#endif // CAMERA_HPP
