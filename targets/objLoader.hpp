#ifndef OBJLOADER_HPP
#define OBJLOADER_HPP

#include <filesystem>
#include <glm/glm.hpp>
#include <string>
#include <unordered_map>
#include <vector>
#include "simpleMesh.hpp"

struct Vec3Hash {
  size_t operator()(const glm::vec3 &v) const {
    size_t h1 = std::hash<float>()(v.x);
    size_t h2 = std::hash<float>()(v.y);
    size_t h3 = std::hash<float>()(v.z);
    return h1 ^ (h2 << 1) ^ (h3 << 2);
  }
};

class OBJLoader {
public:
  /**
   * @brief Load obj file
   * @params filePath the path to file
   */
  void load(const std::string &filePath);

  /**
   * @brief Get vertices vector
   */
  std::vector<Vertex> getVertices() const;

  /**
   * @brief Get indices vector
   */
  std::vector<uint32_t> getIndices() const;

private:
  std::vector<Vertex> m_vertices;
  std::vector<uint32_t> m_indices;

  /**
   * @brief Helper function to parse face vertex data
   * @params vertexStr the string of vertex data point_idx/normal_idx/uv_idx
   * @params vertex vertex to be filled
   * @params positions vector of raw coordinates
   * @params normals vector of raw normal
   * @params uvs vector of raw uv
   * @params color color of Vertex
   */
  void parseFaceVertex(const std::string &vertexStr, Vertex &vertex,
                       const std::vector<glm::vec3> &positions,
                       const std::vector<glm::vec3> &normals);
};

#endif // OBJLOADER_HPP
