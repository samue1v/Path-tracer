#ifndef TRACER_HPP
#define TRACER_HPP

#include <glm/glm.hpp>
#include <stdint.h>

namespace Tracer {

struct alignas(16) PRNG32 {
  glm::uvec4 state;
  float value;
  uint32_t pad[3];
};

struct alignas(16) camera {
  glm::mat4 view;
  glm::mat4 invView;
  glm::vec4 pos;
};

struct alignas(16) hitData {
  glm::vec4 wo;
  glm::vec4 hit;
  glm::vec4 normal;
  glm::vec4 throughput_depth; // w coordinate is depth
};

struct alignas(16) pixelData {
  glm::vec4 color_currentRay;
};

struct alignas(16) Material {
  // TODO Align this struct better

  glm::vec4 albedo_refractiveIdx;
  glm::vec4 emissive_reflectiveIdx;
  glm::vec4 type_pad3;
  
};

struct alignas(16) sphere {
  glm::vec4 center_radius;
  Tracer::Material mat;
};

struct alignas(16) plane {
  glm::vec4 center;
  glm::vec4 edge1;
  glm::vec4 edge2;

  Tracer::Material mat;
  float u;
  float v;
  uint32_t pad[2];
};

struct alignas(16) PushConstants {
  uint32_t rpp;
  uint32_t numSpheres;
  uint32_t numPlanes;
  uint32_t numLights;
};

}; // namespace Tracer

#endif
