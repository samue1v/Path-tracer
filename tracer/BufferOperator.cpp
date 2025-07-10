#include "BufferOperator.hpp"
#include <glm/gtc/matrix_transform.hpp>

void TauswortheOperator::doOperation(Buffer &buffer, size_t size,
                                     VmaAllocator allocator) {
  std::mt19937 gen(rd());
  std::uniform_int_distribution<uint32_t> dist(
      129, std::numeric_limits<uint32_t>::max());

  auto *pPRNG =
      reinterpret_cast<Tracer::PRNG32 *>(buffer.allocationInfo.pMappedData);

  for (size_t i = 0; i < size; ++i) {
    pPRNG[i].state.x = dist(gen);
    pPRNG[i].state.y = dist(gen);
    pPRNG[i].state.z = dist(gen);
    pPRNG[i].state.w = dist(gen);
    pPRNG[i].value = 0.f;
  }
}

MultiJitterOperator::MultiJitterOperator(uint32_t width, uint32_t height,
                                         uint32_t rpp,
                                         const Tracer::camera &camera)
    : range(width, height), _rpp(rpp), cam(camera) {}

void MultiJitterOperator::doOperation(Buffer &buffer, size_t size,
                                      VmaAllocator allocator) {

  std::mt19937 gen(rd());
  std::uniform_real_distribution<float> dist(0.f, 1.f);

  auto *pHitData =
      reinterpret_cast<Tracer::hitData *>(buffer.allocationInfo.pMappedData);

  // Viewport pixel partitio size
  uint32_t vpps = _rpp;

  // Ensure vpps is perfect square and a power of 2
  uint32_t root = static_cast<uint32_t>(std::sqrt(vpps));
  assert(root * root == vpps && vpps != 0 && (vpps & (vpps - 1)) == 0);

  float stride_minor = 1 / (float)vpps;
  float stride_major = 1 / (float)range.x;
  glm::vec4 upper_left(-range.x / 2.f, range.y / 2.f, -cam.vp_dist, 1.f);

  for (uint32_t i = 0; i < range.x * range.y; i++) {
    uint32_t c_major = i % range.x;
    uint32_t r_major = i / range.x;
    float x_major = upper_left.x + c_major * stride_major;
    float y_major = upper_left.y - r_major * stride_major;
    for (uint32_t j = 0; j < vpps; j++) {
      uint32_t c_minor = j % root;
      uint32_t r_minor = j / root;
      float x_minor = x_major + c_minor * stride_minor;
      float y_minor = y_major - r_minor * stride_minor;
      glm::vec4 viewport_hit =
          glm::vec4(x_minor + dist(gen) * stride_minor,
                    y_minor - dist(gen) * stride_minor, -cam.vp_dist, 1.f);

      pHitData[i * vpps + j].hit = viewport_hit;
      pHitData[i * vpps + j].color = glm::vec4(0.0, 0.0, 1.0, 1.0);
      pHitData[i * vpps + j].wo = glm::normalize(viewport_hit - cam.pos);
    }
  }
}

CameraPositionOperator::CameraPositionOperator(Tracer::camera &camera)
    : _camera(camera) {}

void CameraPositionOperator::doOperation(Buffer &buffer, size_t size,
                                         VmaAllocator allocator) {
  auto *pCamera =
      reinterpret_cast<Tracer::camera *>(buffer.allocationInfo.pMappedData);

  pCamera->pos = _camera.pos;
  pCamera->view = _camera.view;
  pCamera->vp_dist = _camera.vp_dist;
}

SphereFillOperator::SphereFillOperator(
    const std::vector<Tracer::sphere> &spheres)
    : _spheres(spheres) {}

void SphereFillOperator::doOperation(Buffer &buffer, size_t size,
                                     VmaAllocator allocator) {
  auto *pSpheres =
      reinterpret_cast<Tracer::sphere *>(buffer.allocationInfo.pMappedData);
  for (uint32_t i = 0; i < _spheres.size(); i++) {
    pSpheres[i].center = _spheres[i].center;
    pSpheres[i].radius = _spheres[i].radius;
  }
}

PlaneFillOperator::PlaneFillOperator(const std::vector<Tracer::plane> &planes)
    : _planes(planes) {}

void PlaneFillOperator::doOperation(Buffer &buffer, size_t size,
                                    VmaAllocator allocator) {
  auto *pPlanes =
      reinterpret_cast<Tracer::plane *>(buffer.allocationInfo.pMappedData);
  for (uint32_t i = 0; i < _planes.size(); i++) {
    pPlanes[i].center = _planes[i].center;
    pPlanes[i].edge1 = _planes[i].edge1;
    pPlanes[i].edge2 = _planes[i].edge2;
    pPlanes[i].u = _planes[i].u;
    pPlanes[i].v = _planes[i].v;
  }
}
