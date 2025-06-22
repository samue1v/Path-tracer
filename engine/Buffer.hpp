#include <vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

class Buffer {
public:
  Buffer() = default;
  void create(vk::Device device, VmaAllocator allocator,
              vk::BufferCreateInfo bufferInfo,
              VmaAllocationCreateFlags allocFlags);
  void cleanUp();

public:
  vk::Device device;
  vk::Buffer buffer;
  VmaAllocator allocator;
  VmaAllocation allocation;
  VmaAllocationInfo allocationInfo;
};
