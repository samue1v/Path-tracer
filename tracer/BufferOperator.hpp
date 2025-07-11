#ifndef BUFFER_OPERATOR
#define BUFFER_OPERATOR

#include "Buffer.hpp"
#include "tracer.hpp"
#include <random>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>
class BufferOperator {
public:
  virtual ~BufferOperator() = default;

  /**
   * @brief Do a preprocessing on buffer data
   * @param logicalDevice Context VkDevice
   * @param cmdPool Context VkCommandPool
   * @param queue Context VkQueue to be submitted
   * @param buffer Buffer to be processed
   * @param size Size of buffer elements in element units
   * @param allocator Context VMA allocator
   */
  virtual void doOperation(vk::Device logicalDevice, vk::CommandPool cmdPool,
                           vk::Queue queue, Buffer &buffer, size_t size,
                           VmaAllocator allocator) = 0;

  void uploadToVRAM(vk::Device logicalDevice, vk::CommandPool cmdPool,
                    vk::Queue queue, VmaAllocator allocator, Buffer &buffer,
                    size_t size, const void *srcData);
};

class TauswortheOperator : public BufferOperator {
public:
  TauswortheOperator() = default;

  void doOperation(vk::Device logicalDevice, vk::CommandPool cmdPool,
                   vk::Queue queue, Buffer &buffer, size_t size,
                   VmaAllocator allocator) override;

public:
  std::random_device rd;
};

class MultiJitterOperator : public BufferOperator {

public:
  MultiJitterOperator(uint32_t width, uint32_t height, uint32_t rpp,
                      const Tracer::camera &camera);

  void doOperation(vk::Device logicalDevice, vk::CommandPool cmdPool,
                   vk::Queue queue, Buffer &buffer, size_t size,
                   VmaAllocator allocator) override;

public:
  std::random_device rd;
  glm::uvec2 range;
  uint32_t _rpp;
  const Tracer::camera &cam;
};

class CameraPositionOperator : public BufferOperator {

public:
  CameraPositionOperator(Tracer::camera &camera);

  void doOperation(vk::Device logicalDevice, vk::CommandPool cmdPool,
                   vk::Queue queue, Buffer &buffer, size_t size,
                   VmaAllocator allocator) override;

public:
  Tracer::camera &_camera;
};

class SphereFillOperator : public BufferOperator {

public:
  SphereFillOperator(const std::vector<Tracer::sphere> &spheres);

  void doOperation(vk::Device logicalDevice, vk::CommandPool cmdPool,
                   vk::Queue queue, Buffer &buffer, size_t size,
                   VmaAllocator allocator) override;

public:
  const std::vector<Tracer::sphere> &_spheres;
};

class PlaneFillOperator : public BufferOperator {

public:
  PlaneFillOperator(const std::vector<Tracer::plane> &planes);

  void doOperation(vk::Device logicalDevice, vk::CommandPool cmdPool,
                   vk::Queue queue, Buffer &buffer, size_t size,
                   VmaAllocator allocator) override;

public:
  const std::vector<Tracer::plane> &_planes;
};

class PixelFillOperator : public BufferOperator {

public:
  PixelFillOperator() = default;

  void doOperation(vk::Device logicalDevice, vk::CommandPool cmdPool,
                   vk::Queue queue, Buffer &buffer, size_t size,
                   VmaAllocator allocator) override;

public:
};

#endif
