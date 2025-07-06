#include "Buffer.hpp"
#include "Logger.hpp"

void Buffer::create(vk::Device _device, VmaAllocator _allocator,
                    vk::BufferCreateInfo bufferInfo,
                    VmaAllocationCreateFlags allocFlags) {
  device = _device;
  allocator = _allocator;

  VmaAllocationCreateInfo allocInfo = {};
  allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
  allocInfo.flags = allocFlags;

  VkBufferCreateInfo bufferInfoC = static_cast<VkBufferCreateInfo>(bufferInfo);

  VkBuffer cbuffer;

  vmaCreateBuffer(allocator, &bufferInfoC, &allocInfo, &cbuffer, &allocation,
                  &allocationInfo);

  buffer = cbuffer;
  Logger::log(
      Logger::LogLevel::DEBUG,
      {"Buffer Alocado!", "Size: " + std::to_string(allocationInfo.size)});
}

void Buffer::cleanUp() { vmaDestroyBuffer(allocator, buffer, allocation); }
