#ifndef TRACER_HPP
#define TRACER_HPP

#include <glm/glm.hpp>
#include <stdint.h>

namespace Tracer {

struct alignas(16) PRNG32 {
  glm::uvec4 state;
};

struct alignas(16) camera {
  glm::mat4 view;
  glm::mat4 invView;
  glm::vec4 pos;
};

struct alignas(16) hitData {
  glm::vec4 wo;
  glm::vec4 hit;
  glm::vec4 throughput_depth; // w coordinate is depth
  glm::vec4 init_wo;
};

struct alignas(16) pixelData {
  glm::vec4 radiance_currentRay;
};

struct alignas(16) Material {
  glm::vec4 albedo_refractiveIdx;
  glm::vec4 emissive_reflectiveIdx;
  glm::vec4 type_pad3;
  // for spheres, type_pad.y = 1 means half sphere
  
};

struct alignas(16) sphere {
  glm::vec4 center_radius;
  Tracer::Material mat;
};

struct alignas(16) plane {
  glm::vec4 center;
  glm::vec4 edge1;
  glm::vec4 edge2;
  glm::vec4 uv_pad2;
  Tracer::Material mat;
};

struct alignas(16) PushConstants {
  glm::mat4 m;
  uint32_t rpp;
  uint32_t numSpheres;
  uint32_t numPlanes;
  uint32_t numLights;
  bool camera_move;
};

}; // namespace Tracer

#endif
