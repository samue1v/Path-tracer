#ifndef SHARED_STRUCTS_HPP
#define SHARED_STRUCTS_HPP
#include <glm/matrix.hpp>
namespace Shared {

  struct CameraMatrices{
    glm::mat4 view;
    glm::mat4 proj;
  };

}

#endif
