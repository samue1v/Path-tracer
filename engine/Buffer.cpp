#include "Buffer.hpp"
#include "Logger.hpp"

void Buffer::create(vk::Device _device, VmaAllocator _allocator,
                    vk::BufferCreateInfo bufferInfo, VmaMemoryUsage usage,
                    VkMemoryPropertyFlags allocFlags) {
  device = _device;
  allocator = _allocator;

  VmaAllocationCreateInfo allocInfo = {};
  allocInfo.usage = usage;
  allocInfo.requiredFlags = allocFlags;

  const VkBufferCreateInfo *bufferInfoC =
      reinterpret_cast<const VkBufferCreateInfo *>(&bufferInfo);

  VkBuffer cbuffer;

  vmaCreateBuffer(allocator, bufferInfoC, &allocInfo, &cbuffer, &allocation,
                  &allocationInfo);

  buffer = cbuffer;
  Logger::log(
      Logger::LogLevel::DEBUG,
      {"Buffer Alocado!", "Size: " + std::to_string(allocationInfo.size)});
  Logger::log(Logger::LogLevel::DEBUG,
              "Offset: " + std::to_string(allocationInfo.offset));
}

void Buffer::cleanUp() { vmaDestroyBuffer(allocator, buffer, allocation); }
