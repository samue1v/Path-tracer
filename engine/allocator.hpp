#ifndef ALLOCATOR_HPP
#define ALLOCATOR_HPP
#include "simpleMesh.hpp"
#include <deque>
#include <functional>
#include <vector>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

struct MeshAllocation {
  VmaVirtualAllocation vertexAlloc = VK_NULL_HANDLE;
  VmaVirtualAllocation indexAlloc = VK_NULL_HANDLE;
  VmaAllocationInfo vertexAllocInfo = {};
  VkDeviceSize vertexOffset = 0;
  VkDeviceSize indexOffset = 0;
  uint32_t vertexCount = 0;
  uint32_t indexCount = 0;
  bool isVisible = true;
  bool isValid = false;
  Mesh mesh;
};

class Allocator {
public:
  ~Allocator();
  void initialize(vk::Device logicalDevice, VmaAllocator allocator, VkDeviceSize initialVertexSize,
                  VkDeviceSize initialIndexSize);
  MeshAllocation allocate(const Mesh &mesh);
  void free(MeshAllocation &allocation);
  void upload(const MeshAllocation &allocation,
              const std::vector<Vertex> &vertices,
              const std::vector<uint32_t> &indices);
  void allocUniformBuffers(vk::DeviceSize size, int framesInFlight);
  void cleanUp();

  // Render helpers
  vk::Buffer getVertexBuffer() const { return vertexBuffer; }
  vk::Buffer getIndexBuffer() const { return indexBuffer; }

private:
  void createBuffers(VkDeviceSize vertexSize, VkDeviceSize indexSize);
  void destroyBuffers();

public:
  vk::Device logicalDevice;
  VmaAllocator allocator;
  vk::Buffer vertexBuffer;
  vk::Buffer indexBuffer;
  VmaAllocation vertexAllocation;
  VmaAllocationInfo vertexAllocationInfo;
  VmaAllocation indexAllocation;
  VmaAllocationInfo indexAllocationInfo;
  VmaVirtualBlock vertexVirtualBlock;
  VmaVirtualBlock indexVirtualBlock;

  std::deque<std::function<void(vk::Device)>> deletionQueue;

  std::vector<vk::Buffer> uniformBuffers;
  std::vector<VmaAllocation> uniformAllocations;
  std::vector<void *> uniformBuffersMapped;
};

#endif // ALLOCATOR_HPP
