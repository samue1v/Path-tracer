#include "ResourceImage.hpp"
#include "Logger.hpp"
void ResourceImage::createImage(vk::Device device, VmaAllocator memAllocator,
                                const vk::ImageCreateInfo &createInfo) {
  allocator = memAllocator;
  device_ = device;
  format_ = createInfo.format;
  extent_ = createInfo.extent;
  usage_ = createInfo.usage;
  // Temporary turnaround, still have to find a better way of doing that
  currentLayout_ = usage_ & vk::ImageUsageFlagBits::eStorage
                       ? vk::ImageLayout::eGeneral
                       : vk::ImageLayout::eUndefined;

  VkImageCreateInfo cInfo = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
      .pNext = nullptr,
      .flags = static_cast<VkImageCreateFlags>(createInfo.flags),
      .imageType = static_cast<VkImageType>(createInfo.imageType),
      .format = static_cast<VkFormat>(createInfo.format),
      .extent = {.width = createInfo.extent.width,
                 .height = createInfo.extent.height,
                 .depth = createInfo.extent.depth},
      .mipLevels = createInfo.mipLevels,
      .arrayLayers = createInfo.arrayLayers,
      .samples = static_cast<VkSampleCountFlagBits>(createInfo.samples),
      .tiling = static_cast<VkImageTiling>(createInfo.tiling),
      .usage = static_cast<VkImageUsageFlags>(createInfo.usage),
      .sharingMode = static_cast<VkSharingMode>(createInfo.sharingMode),
      .queueFamilyIndexCount = createInfo.queueFamilyIndexCount,
      .pQueueFamilyIndices = createInfo.pQueueFamilyIndices,
      .initialLayout = static_cast<VkImageLayout>(createInfo.initialLayout)};

  VmaAllocationCreateInfo allocCreateInfo = {};
  allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

  VkResult res = vmaCreateImage(allocator, &cInfo, &allocCreateInfo, &image_,
                                &alloc, &allocInfo);

  assert(res == VK_SUCCESS);
  Logger::log(
      Logger::LogLevel::DEBUG,
      {"Storage image allocated.", "Size: " + std::to_string(allocInfo.size)});
}

void ResourceImage::transitionLayout(
    vk::CommandBuffer cmd, const std::array<uint32_t, 2> &queueIndices,
    vk::ImageLayout oldLayout, vk::ImageLayout newLayout,
    vk::AccessFlags srcAccess, vk::AccessFlags dstAccess,
    vk::PipelineStageFlags srcStage, vk::PipelineStageFlags dstStage) {
  vk::ImageMemoryBarrier barrier = {};
  barrier.oldLayout = oldLayout;
  barrier.newLayout = newLayout;
  barrier.srcQueueFamilyIndex = queueIndices[0];
  barrier.dstQueueFamilyIndex = queueIndices[1];
  barrier.srcAccessMask = srcAccess;
  barrier.dstAccessMask = dstAccess;
  barrier.image = image_;
  barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;

  cmd.pipelineBarrier(srcStage, // Adjust as needed
                      dstStage, vk::DependencyFlags(), nullptr, nullptr,
                      barrier);
}

void ResourceImage::createView(vk::ImageAspectFlags aspectFlags) {
  vk::ImageViewCreateInfo viewInfo{};
  viewInfo.sType = vk::StructureType::eImageViewCreateInfo;
  viewInfo.image = image_;
  viewInfo.viewType = vk::ImageViewType::e2D;
  viewInfo.format = format_;
  viewInfo.subresourceRange.aspectMask = aspectFlags;
  viewInfo.subresourceRange.baseMipLevel = 0;
  viewInfo.subresourceRange.levelCount = 1;
  viewInfo.subresourceRange.baseArrayLayer = 0;
  viewInfo.subresourceRange.layerCount = 1;

  view_ = device_.createImageView(viewInfo);
  assert(view_ != VK_NULL_HANDLE);
}

void ResourceImage::cleanUp() {
  if (image_ != VK_NULL_HANDLE) {
    vmaDestroyImage(allocator, image_, alloc);
    device_.destroyImageView(view_);
  }
}
