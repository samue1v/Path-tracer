#include "objLoader.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <unistd.h>

void OBJLoader::load(const std::string &filePath) {
  m_vertices.clear();
  m_indices.clear();

  std::vector<glm::vec3> temp_positions;
  std::vector<glm::vec3> temp_normals;

  // Map to store unique vertices based ONLY on position
  std::unordered_map<glm::vec3, uint32_t, Vec3Hash> uniqueVertices;

  // Get file path (your existing path resolution code)
  char buffer[1024];
  ssize_t count = readlink("/proc/self/exe", buffer, sizeof(buffer));
  if (count == -1)
    throw std::runtime_error("Cannot read /proc/self/exe");
  auto finalPath = std::filesystem::path(std::string(buffer, count))
                       .parent_path()
                       .parent_path() /
                   "meshFiles" / filePath;

  std::ifstream file(finalPath);
  if (!file.is_open()) {
    throw std::runtime_error("Unable to open file: " + finalPath.string());
  }

  std::string line;
  while (std::getline(file, line)) {
    std::istringstream iss(line);
    std::string type;
    iss >> type;

    if (type == "v") {
      glm::vec3 pos;
      iss >> pos.x >> pos.y >> pos.z;
      temp_positions.push_back(pos);
    } else if (type == "vn") {
      glm::vec3 normal;
      iss >> normal.x >> normal.y >> normal.z;
      temp_normals.push_back(normal);
    } else if (type == "f") {
      std::string v[3];
      iss >> v[0] >> v[1] >> v[2];

      for (int i = 0; i < 3; i++) {
        Vertex vertex;
        parseFaceVertex(v[i], vertex, temp_positions, temp_normals);

        // Check if this position already exists
        auto it = uniqueVertices.find(vertex.pos);
        if (it == uniqueVertices.end()) {
          // New vertex - add to storage and map
          uint32_t index = static_cast<uint32_t>(m_vertices.size());
          uniqueVertices[vertex.pos] = index;
          m_vertices.push_back(vertex);
          m_indices.push_back(index);
        } else {
          // Existing vertex - just add index
          m_indices.push_back(it->second);
        }
      }
    }
  }
  std::cout << "VERTEX SIZE: " << m_vertices.size() << std::endl;
}

void OBJLoader::parseFaceVertex(const std::string &vertexStr, Vertex &vertex,
                                const std::vector<glm::vec3> &positions,
                                const std::vector<glm::vec3> &normals) {
  std::istringstream viss(vertexStr);
  std::string pos_str, normal_str;

  // Parse position/uv/normal indices (format: pos/uv/normal)
  if (!std::getline(viss, pos_str, '/')) {
    throw std::runtime_error("Could not parse position from face");
  }
  std::string uv_dummy;
  std::getline(viss, uv_dummy, '/');
  std::getline(viss, normal_str, '/');

  int pos_idx;
  try {
    pos_idx = std::stoi(pos_str) - 1;
  } catch (...) {
    throw std::runtime_error("Pos_idx in NAN");
  }

  if (pos_idx < 0 || pos_idx >= positions.size()) {
    throw std::runtime_error("Face index out of bounds");
  }
  vertex.pos = {positions[pos_idx]};

  if (!normal_str.empty()) {
    int normal_idx;
    try {
      normal_idx = std::stoi(normal_str) - 1;
    } catch (...) {

      throw std::runtime_error("normal_idx is NAN");
    }

    if (normal_idx < 0 || normal_idx >= normals.size()) {
      throw std::runtime_error("normal index out of bounds");
    }
    vertex.normal = normals[normal_idx];
  } else {
    vertex.normal = glm::vec3(0.0f);
  }

}

std::vector<Vertex> OBJLoader::getVertices() const { return m_vertices; }

std::vector<uint32_t> OBJLoader::getIndices() const { return m_indices; }
