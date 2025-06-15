#define VMA_IMPLEMENTATION
#include "allocator.hpp"
#include <cstring>
#include <stdexcept>

Allocator::~Allocator() {}

void Allocator::initialize(vk::Device logicalDevice, VmaAllocator allocator,
                           VkDeviceSize initialVertexSize,
                           VkDeviceSize initialIndexSize) {

  this->allocator = allocator;
  this->logicalDevice = logicalDevice;
  // Create virtual blocks
  VmaVirtualBlockCreateInfo blockInfo = {};
  blockInfo.size = initialVertexSize;
  if (vmaCreateVirtualBlock(&blockInfo, &vertexVirtualBlock) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create vertex virtual block");
  }

  blockInfo.size = initialIndexSize;
  if (vmaCreateVirtualBlock(&blockInfo, &indexVirtualBlock) != VK_SUCCESS) {
    vmaDestroyVirtualBlock(vertexVirtualBlock);
    throw std::runtime_error("Failed to create index virtual block");
  }

  // Create GPU buffers
  createBuffers(initialVertexSize, initialIndexSize);

  deletionQueue.push_back([this](vk::Device device) {
    this->destroyBuffers();
    vmaClearVirtualBlock(vertexVirtualBlock);
    vmaClearVirtualBlock(indexVirtualBlock);

    vmaDestroyVirtualBlock(vertexVirtualBlock);
    vmaDestroyVirtualBlock(indexVirtualBlock);
    vmaDestroyAllocator(this->allocator);
  });
}

void Allocator::cleanUp() {

  while (deletionQueue.size() > 0) {
    deletionQueue.back()(logicalDevice);
    deletionQueue.pop_back();
  }
}

MeshAllocation Allocator::allocate(const Mesh &mesh) {
  MeshAllocation alloc;
  const VkDeviceSize vertexSize = mesh.vertices.size() * sizeof(Vertex);
  const VkDeviceSize indexSize = mesh.indices.size() * sizeof(uint32_t);

  // Allocate vertex region
  VmaVirtualAllocationCreateInfo allocInfo = {};
  allocInfo.size = vertexSize;
  allocInfo.alignment = alignof(Vertex);
  if (vmaVirtualAllocate(vertexVirtualBlock, &allocInfo, &alloc.vertexAlloc,
                         &alloc.vertexOffset) != VK_SUCCESS) {
    throw std::runtime_error("Failed to allocate vertex memory");
  }

  // Allocate index region
  allocInfo.size = indexSize;
  allocInfo.alignment = alignof(uint32_t);
  if (vmaVirtualAllocate(indexVirtualBlock, &allocInfo, &alloc.indexAlloc,
                         &alloc.indexOffset) != VK_SUCCESS) {
    vmaVirtualFree(vertexVirtualBlock, alloc.vertexAlloc);
    throw std::runtime_error("Failed to allocate index memory");
  }

  alloc.mesh = mesh;
  alloc.vertexCount = static_cast<uint32_t>(mesh.vertices.size());
  alloc.indexCount = static_cast<uint32_t>(mesh.indices.size());
  alloc.isValid = true;
  alloc.isVisible = true;

  // Upload data
  upload(alloc, mesh.vertices, mesh.indices);

  return alloc;
}

void Allocator::free(MeshAllocation &allocation) {
  if (allocation.isValid) {
    vmaVirtualFree(vertexVirtualBlock, allocation.vertexAlloc);
    vmaVirtualFree(indexVirtualBlock, allocation.indexAlloc);
    allocation.isValid = false;
  }
}

void Allocator::upload(const MeshAllocation &allocation,
                       const std::vector<Vertex> &vertices,
                       const std::vector<uint32_t> &indices) {
  void *vertexData;
  void *indexData;
  vmaMapMemory(allocator, vertexAllocation, &vertexData);
  vmaMapMemory(allocator, indexAllocation, &indexData);

  // Copy to GPU
  memcpy(static_cast<char *>(vertexData) + allocation.vertexOffset,
         vertices.data(), vertices.size() * sizeof(Vertex));
  memcpy(static_cast<char *>(indexData) + allocation.indexOffset,
         indices.data(), indices.size() * sizeof(uint32_t));

  // Flush if memory is not coherent
  VmaAllocationInfo allocInfo;
  vmaGetAllocationInfo(allocator, vertexAllocation, &allocInfo);
  if (!(allocInfo.memoryType & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
    vmaFlushAllocation(allocator, vertexAllocation, allocation.vertexOffset,
                       vertices.size() * sizeof(Vertex));
    vmaFlushAllocation(allocator, indexAllocation, allocation.indexOffset,
                       indices.size() * sizeof(uint32_t));
  }

  vmaUnmapMemory(allocator, vertexAllocation);
  vmaUnmapMemory(allocator, indexAllocation);
}

void Allocator::createBuffers(VkDeviceSize vertexSize, VkDeviceSize indexSize) {
  // Vertex buffer
  vk::BufferCreateInfo vertexBufferInfo = {};
  vertexBufferInfo.size = vertexSize;
  vertexBufferInfo.usage = vk::BufferUsageFlagBits::eVertexBuffer |
                           vk::BufferUsageFlagBits::eTransferDst;
  vertexBufferInfo.sharingMode = vk::SharingMode::eExclusive;

  VmaAllocationCreateInfo vertexAllocInfo = {};
  vertexAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
  vertexAllocInfo.flags =
      VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
      VMA_ALLOCATION_CREATE_MAPPED_BIT;

  if (vmaCreateBuffer(
          allocator,
          reinterpret_cast<const VkBufferCreateInfo *>(&vertexBufferInfo),
          &vertexAllocInfo, reinterpret_cast<VkBuffer *>(&vertexBuffer),
          &vertexAllocation, &vertexAllocationInfo) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create vertex buffer");
  }

  // Index buffer
  vk::BufferCreateInfo indexBufferInfo = {};
  indexBufferInfo.size = indexSize;
  indexBufferInfo.usage = vk::BufferUsageFlagBits::eIndexBuffer |
                          vk::BufferUsageFlagBits::eTransferDst;
  indexBufferInfo.sharingMode = vk::SharingMode::eExclusive;

  VmaAllocationCreateInfo indexAllocInfo = {};
  indexAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
  indexAllocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

  if (vmaCreateBuffer(
          allocator,
          reinterpret_cast<const VkBufferCreateInfo *>(&indexBufferInfo),
          &indexAllocInfo, reinterpret_cast<VkBuffer *>(&indexBuffer),
          &indexAllocation, &indexAllocationInfo) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create index buffer");
  }
}

void Allocator::allocUniformBuffers(vk::DeviceSize size, int framesInFlight) {
  VkDeviceSize bufferSize = size;

  uniformBuffers.resize(framesInFlight);
  uniformAllocations.resize(framesInFlight);
  uniformBuffersMapped.resize(framesInFlight);

  for (size_t i = 0; i < framesInFlight; i++) {
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = bufferSize;
    bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                      VMA_ALLOCATION_CREATE_MAPPED_BIT;

    VmaAllocationInfo allocationInfo{};
    vmaCreateBuffer(allocator, &bufferInfo, &allocInfo,
                    reinterpret_cast<VkBuffer *>(&uniformBuffers[i]),
                    &uniformAllocations[i], &allocationInfo);

    uniformBuffersMapped[i] = allocationInfo.pMappedData;
  }
}


void Allocator::destroyBuffers() {
  if (vertexBuffer) {
    vmaDestroyBuffer(allocator, vertexBuffer, vertexAllocation);
    vertexBuffer = nullptr;
  }
  if (indexBuffer) {
    vmaDestroyBuffer(allocator, indexBuffer, indexAllocation);
    indexBuffer = nullptr;
  }

  for (int i = 0; i < uniformBuffers.size(); i++) {
    vmaDestroyBuffer(allocator, uniformBuffers[i], uniformAllocations[i]);
    uniformBuffers[i] = nullptr;
  }
}
