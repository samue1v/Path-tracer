#ifndef CAMERA_HPP
#define CAMERA_HPP

#include "tracer.hpp"
#include <GLFW/glfw3.h>
#include <atomic>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_projection.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/glm.hpp>
#include <glm/matrix.hpp>
#include <glm/vec4.hpp>
#include <array>

static float speed = 20.f;
static float xsens = .05f;
static float ysens = .05f;

class Camera {
public:
  Camera(GLFWwindow *glfw_window, std::atomic<int> &readIndex,
         std::atomic<int> &writeIndex,
         std::array<Tracer::camera, 2> &bufferRef, std::atomic<bool> &cameraMoved);

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

  /**
   * @brief Walk back
   */
  void walkBack();

  /**
   * @brief Walk front
   */
  void walkFront();

  /**
   * @brief Walk left
   */
  void walkLeft();

  /**
   * @brief Walk right
   */
  void walkRight();

/**
 * @brief Shake camera
 * @param xoffset offset of x axis
 * @param yoffset offset of y axis
 */
  void shake(float xoffset, float yoffset);

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
  std::atomic<bool> &cameraMoved;
  std::array<Tracer::camera, 2> &buffer_camera_;
  Tracer::camera camera_;
  std::atomic<float> deltaTime;
};

#endif // CAMERA_HPP
