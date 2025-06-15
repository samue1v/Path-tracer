#ifndef OBJECT_HPP
#define OBJECT_HPP
#define VULKAN_HPP_NO_EXCEPTIONS
#include <glm/vec3.hpp>
#include <string>
#include <vector>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

struct Vertex {
  glm::vec3 pos;
  glm::vec3 normal;
};

struct Face {
  uint32_t points[3];
  glm::vec3 normal;
  float d;
};

class Mesh {
public:
  /**
   * @brief Mesh name
   */
  std::string name;

  /**
   * @brief Vector of Base Vertex * to a specific Vertex implementation
   */
  std::vector<Vertex> vertices;

  /**
   * @brief Indicies of vertices
   */
  std::vector<uint32_t> indices;

  /**
   * @brief Faces
   */
  std::vector<Face> faces;
};

#endif
