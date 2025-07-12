#ifndef CAMERA_HPP
#define CAMERA_HPP

#include "tracer.hpp"
#include <atomic>
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
  Camera(std::atomic<int> &readIndex,
         std::atomic<int> &writeIndex,
         std::array<Tracer::camera, 2> &bufferRef);

  void resetView();

  void updateView();

  void update();

public:
  glm::vec3 at;
  glm::vec3 pos;
  glm::vec3 right;
  glm::vec3 up;
  glm::vec3 worldUp;
  float yaw;
  float pitch;

  float fov;

  std::atomic<int> &readIndex;
  std::atomic<int> &writeIndex;
  std::array<Tracer::camera, 2> &buffer;
};

#endif // CAMERA_HPP
