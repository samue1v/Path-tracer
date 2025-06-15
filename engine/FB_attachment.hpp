#ifndef FB_ATTACHMENT_HPP
#define FB_ATTACHMENT_HPP

#include <deque>
#include <functional>
#define VULKAN_HPP_NO_EXCEPTIONS
#include "frame.hpp"
#include <unordered_map>
#include <vector>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

class FB_Attachment {
public:
  FB_Attachment() = default;
  FB_Attachment(vk::Device logicalDevice, VmaAllocator & allocator);
  void createAttachment(vk::Format format, vk::Extent2D extent);
  void cleanUp();

public:
  Frame frame;
  VmaAllocation allocation;
  std::deque<std::function<void(vk::Device)>> deletionQueue;
  vk::Device logicalDevice;
  VmaAllocator allocator;
};

#endif
