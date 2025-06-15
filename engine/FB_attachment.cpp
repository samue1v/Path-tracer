//#define VMA_IMPLEMENTATION
#include "FB_attachment.hpp"

FB_Attachment::FB_Attachment(vk::Device logicalDevice, VmaAllocator &allocator)
    : logicalDevice(logicalDevice), allocator(allocator) {}

void FB_Attachment::createAttachment(vk::Format format, vk::Extent2D extent) {
  vk::ImageCreateInfo imageInfo = {};
  imageInfo.imageType = vk::ImageType::e2D;
  imageInfo.extent = vk::Extent3D{extent.width, extent.height, 1};
  imageInfo.mipLevels = 1;
  imageInfo.arrayLayers = 1;
  imageInfo.format = format;
  imageInfo.tiling = vk::ImageTiling::eOptimal;
  imageInfo.initialLayout = vk::ImageLayout::eUndefined;
  imageInfo.usage = vk::ImageUsageFlagBits::eDepthStencilAttachment;
  imageInfo.samples = vk::SampleCountFlagBits::e1;
  imageInfo.sharingMode = vk::SharingMode::eExclusive;

  VmaAllocationCreateInfo allocInfo = {};
  allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

  VkImage rawImage;
  VmaAllocationInfo allocationInfo;

  VkResult result = vmaCreateImage(

      allocator, reinterpret_cast<const VkImageCreateInfo *>(&imageInfo),
      &allocInfo, &rawImage, &allocation, &allocationInfo);

  if (result != VK_SUCCESS) {
    throw std::runtime_error("Failed to create image with VMA");
  }
  frame.image = vk::Image(rawImage);

  vk::ImageViewCreateInfo viewInfo{};
  viewInfo.sType = vk::StructureType::eImageViewCreateInfo;
  viewInfo.image = frame.image;
  viewInfo.viewType = vk::ImageViewType::e2D;
  viewInfo.format = format;
  viewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;
  viewInfo.subresourceRange.baseMipLevel = 0;
  viewInfo.subresourceRange.levelCount = 1;
  viewInfo.subresourceRange.baseArrayLayer = 0;
  viewInfo.subresourceRange.layerCount = 1;

  auto resView = logicalDevice.createImageView(viewInfo);
  if (resView.result != vk::Result::eSuccess) {
    throw std::runtime_error("Failed to allocate depth image view");
  }
  frame.imageView = resView.value;

  deletionQueue.push_back([this](vk::Device device) {
    device.destroyImageView(this->frame.imageView);
    vmaDestroyImage(allocator, frame.image, allocation);
  });
}

void FB_Attachment::cleanUp() {
  while (deletionQueue.size() > 0) {
    deletionQueue.back()(logicalDevice);
    deletionQueue.pop_back();
  }
}
