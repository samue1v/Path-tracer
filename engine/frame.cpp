#include "frame.hpp"

Frame::Frame(vk::Image image, vk::Device logicalDevice,
             vk::Format swapchainFormat)
  : image(image), format(swapchainFormat), layout(vk::ImageLayout::eUndefined){

    createImageView(logicalDevice);

  }

void Frame::createImageView(vk::Device logicalDevice) {
  vk::ImageViewCreateInfo createInfo;
  createInfo.image = image;
  createInfo.viewType = vk::ImageViewType::e2D;
  createInfo.format = format;
  createInfo.components.r = vk::ComponentSwizzle::eIdentity;
  createInfo.components.g = vk::ComponentSwizzle::eIdentity;
  createInfo.components.b = vk::ComponentSwizzle::eIdentity;
  createInfo.components.a = vk::ComponentSwizzle::eIdentity;
  createInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
  createInfo.subresourceRange.baseMipLevel = 0;
  createInfo.subresourceRange.levelCount = 1;
  createInfo.subresourceRange.baseArrayLayer = 0;
  createInfo.subresourceRange.layerCount = 1;

  imageView = logicalDevice.createImageView(createInfo).value;
}
