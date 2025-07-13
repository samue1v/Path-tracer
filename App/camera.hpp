#ifndef CAMERA_HPP
#define CAMERA_HPP

#include "tracer.hpp"
#include <atomic>
#include <GLFW/glfw3.h>
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
  Camera(GLFWwindow * glfw_window, std::atomic<int> &readIndex,
         std::atomic<int> &writeIndex,
         std::array<Tracer::camera, 2> &bufferRef);

  /**
   * @brief Reset camera top initial state
   */
  void resetView();

  /**
   * @brief Update Tracer::camera scene camera from current pos, at and up
   * parameters
   */
  void updateView();

  /**
   * @brief Update and Sync logic
   */
  void update();

public:
  glm::vec3 at;
  glm::vec3 pos;
  glm::vec3 right;
  glm::vec3 up;
  glm::ivec2 vp_size;
  float yaw;
  float pitch;

  float fov;

  std::atomic<int> &readIndex;
  std::atomic<int> &writeIndex;
  std::array<Tracer::camera, 2> &buffer;
  Tracer::camera camera;
};

#endif // CAMERA_HPP
