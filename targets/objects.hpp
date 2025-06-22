#ifndef OBJECTS_HPP
#define OBJECTS_HPP
#include <glm/vec3.hpp>

struct alignas(16) Sphere{
  glm::vec3 center;
  float radius;
};

struct alignas(16) Plane{
  glm::vec3 p0;
  float d;
};

#endif
