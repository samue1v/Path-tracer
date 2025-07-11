#include "BufferOperator.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include "Logger.hpp"

void BufferOperator::uploadToVRAM(vk::Device logicalDevice,
                                  vk::CommandPool cmdPool, vk::Queue queue,
                                  VmaAllocator allocator, Buffer &buffer,
                                  size_t size, const void *srcData) {
  Logger::log(Logger::LogLevel::DEBUG, "COPY SIZE: " + std::to_string(size));
  VkBuffer stagingBuffer;
  VmaAllocation stagingAllocation;

  VkBufferCreateInfo bufferInfo{};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = size;
  bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

  VmaAllocationCreateInfo allocInfo{};
  allocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
  allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                    VMA_ALLOCATION_CREATE_MAPPED_BIT;

  if (vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &stagingBuffer,
                      &stagingAllocation, nullptr) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create staging buffer");
  }

  void *mapped = nullptr;
  vmaMapMemory(allocator, stagingAllocation, &mapped);
  std::memcpy(mapped, srcData, static_cast<size_t>(buffer.allocationInfo.size));
  vmaUnmapMemory(allocator, stagingAllocation);

  vk::CommandBufferAllocateInfo allocCmdInfo{};
  allocCmdInfo.sType = vk::StructureType::eCommandBufferAllocateInfo;
  allocCmdInfo.level = vk::CommandBufferLevel::ePrimary;
  allocCmdInfo.commandPool = cmdPool;
  allocCmdInfo.commandBufferCount = 1;

  vk::CommandBuffer commandBuffer;
  commandBuffer = logicalDevice.allocateCommandBuffers(allocCmdInfo)[0];

  vk::CommandBufferBeginInfo beginInfo{};
  beginInfo.sType = vk::StructureType::eCommandBufferBeginInfo;
  beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

  commandBuffer.begin(&beginInfo);

  vk::BufferCopy copyRegion{};
  copyRegion.srcOffset = 0;
  copyRegion.dstOffset = 0;
  copyRegion.size = size;

  vk::Buffer stagingBufferWrapper = stagingBuffer;

  commandBuffer.copyBuffer(stagingBufferWrapper, buffer.buffer, 1, &copyRegion);

  commandBuffer.end();

  vk::SubmitInfo submitInfo{};
  submitInfo.sType = vk::StructureType::eSubmitInfo;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;

  queue.submit(submitInfo);
  queue.waitIdle();

  logicalDevice.freeCommandBuffers(cmdPool, commandBuffer);
  vmaDestroyBuffer(allocator, stagingBuffer, stagingAllocation);
}

void TauswortheOperator::doOperation(vk::Device logicalDevice,
                                     vk::CommandPool cmdPool, vk::Queue queue,
                                     Buffer &buffer, size_t size,
                                     VmaAllocator allocator) {
  std::mt19937 gen(rd());
  std::uniform_int_distribution<uint32_t> dist(
      129, std::numeric_limits<uint32_t>::max());

  std::vector<Tracer::PRNG32> pPRNG(size);

  for (size_t i = 0; i < size; ++i) {
    pPRNG[i].state.x = dist(gen);
    pPRNG[i].state.y = dist(gen);
    pPRNG[i].state.z = dist(gen);
    pPRNG[i].state.w = dist(gen);
    pPRNG[i].value = 0.f;
  }

  uploadToVRAM(logicalDevice, cmdPool, queue, allocator, buffer,
               size * sizeof(Tracer::PRNG32), pPRNG.data());
}

MultiJitterOperator::MultiJitterOperator(uint32_t width, uint32_t height,
                                         uint32_t rpp,
                                         const Tracer::camera &camera)
    : range(width, height), _rpp(rpp), cam(camera) {}

void MultiJitterOperator::doOperation(vk::Device logicalDevice,
                                      vk::CommandPool cmdPool, vk::Queue queue,
                                      Buffer &buffer, size_t size,
                                      VmaAllocator allocator) {

  std::mt19937 gen(rd());
  std::uniform_real_distribution<float> dist(0.f, 1.f);

  std::vector<Tracer::hitData> pHitData(size);

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
      pHitData[i * vpps + j].wo = glm::normalize(viewport_hit - cam.pos);
    }
  }

  uploadToVRAM(logicalDevice, cmdPool, queue, allocator, buffer,
               size * sizeof(Tracer::hitData), pHitData.data());
}

CameraPositionOperator::CameraPositionOperator(Tracer::camera &camera)
    : _camera(camera) {}

void CameraPositionOperator::doOperation(vk::Device logicalDevice,
                                         vk::CommandPool cmdPool,
                                         vk::Queue queue, Buffer &buffer,
                                         size_t size, VmaAllocator allocator) {
  std::vector<Tracer::camera> pCamera(size);

  pCamera[0].pos = _camera.pos;
  pCamera[0].view = _camera.view;
  pCamera[0].vp_dist = _camera.vp_dist;
  
  uploadToVRAM(logicalDevice, cmdPool, queue, allocator, buffer,
               size * sizeof(Tracer::camera), pCamera.data());
}

SphereFillOperator::SphereFillOperator(
    const std::vector<Tracer::sphere> &spheres)
    : _spheres(spheres) {}

void SphereFillOperator::doOperation(vk::Device logicalDevice,
                                     vk::CommandPool cmdPool, vk::Queue queue,
                                     Buffer &buffer, size_t size,
                                     VmaAllocator allocator) {

  std::vector<Tracer::sphere> pSpheres(size);
  for (uint32_t i = 0; i < _spheres.size(); i++) {
    pSpheres[i].center = _spheres[i].center;
    pSpheres[i].radius = _spheres[i].radius;
  }

  uploadToVRAM(logicalDevice, cmdPool, queue, allocator, buffer,
               size * sizeof(Tracer::sphere), pSpheres.data());
}

PlaneFillOperator::PlaneFillOperator(const std::vector<Tracer::plane> &planes)
    : _planes(planes) {}

void PlaneFillOperator::doOperation(vk::Device logicalDevice,
                                    vk::CommandPool cmdPool, vk::Queue queue,
                                    Buffer &buffer, size_t size,
                                    VmaAllocator allocator) {

  std::vector<Tracer::plane> pPlanes(size);

  for (uint32_t i = 0; i < _planes.size(); i++) {
    pPlanes[i].center = _planes[i].center;
    pPlanes[i].edge1 = _planes[i].edge1;
    pPlanes[i].edge2 = _planes[i].edge2;
    pPlanes[i].u = _planes[i].u;
    pPlanes[i].v = _planes[i].v;
  }

  uploadToVRAM(logicalDevice, cmdPool, queue, allocator, buffer,
               size * sizeof(Tracer::plane), pPlanes.data());
}
