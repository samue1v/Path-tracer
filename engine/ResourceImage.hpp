#ifndef RESOURCE_IMAGE_HPP
#define RESOURCE_IMAGE_HPP
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

class ResourceImage {
public:
  ResourceImage() = default;

  void createImage(vk::Device device, VmaAllocator memAllocator,
                   const vk::ImageCreateInfo &createInfo);
  void cleanUp();

  // Accessors
  vk::Image getImage() { return image_; }
  void createView(vk::ImageAspectFlags aspectFlags);

  void transitionLayout(vk::CommandBuffer cmdBuffer,
                        const std::array<uint32_t, 2> &queueIndices,
                        vk::ImageLayout oldLayout, vk::ImageLayout newLayout,
                        vk::AccessFlags srcAccess, vk::AccessFlags dstAccess,
                        vk::PipelineStageFlags srcStage,
                        vk::PipelineStageFlags dstStage);

public:
  vk::Device device_;
  VkImage image_;
  vk::ImageView view_;
  vk::Extent3D extent_;
  vk::Format format_;
  vk::ImageUsageFlags usage_;
  vk::ImageLayout currentLayout_;
  VmaAllocator allocator;
  VmaAllocation alloc;
  VmaAllocationInfo allocInfo;
};

#endif
