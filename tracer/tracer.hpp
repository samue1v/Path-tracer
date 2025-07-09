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
    
    struct camera {
      alignas(16) glm::mat4 view;
      alignas(16) glm::mat4 proj;
    };
    
    struct alignas(16) hitData {
      glm::vec4 wo;
      glm::vec4 wi;
      glm::vec4 hit;
      glm::vec4 normal;
      glm::vec4 color;
      uint32_t depth;
      uint32_t pad[3];
    };
    
    struct alignas(16) data {
      uint32_t numSpheres;
      uint32_t numPlanes;
      uint32_t numLights;
      uint32_t numRays;
      uint32_t maxBounces;
      uint32_t pad[3];
    };

    struct PushConstants {
      glm::vec4 test_color;
      uint32_t rpp;
    };
}; // namespace tracer

#endif
