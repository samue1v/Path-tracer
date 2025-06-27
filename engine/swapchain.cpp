#include "swapchain.hpp"

void Swapchain::create(vk::Device logicalDevice,
                       vk::PhysicalDevice physicalDevice,
                       vk::SurfaceKHR surface, uint32_t width,
                       uint32_t height) {
  this->logicalDevice = logicalDevice;

  vk::SurfaceCapabilitiesKHR capabilities =
      physicalDevice.getSurfaceCapabilitiesKHR(surface).value;

  std::vector<vk::SurfaceFormatKHR> formats =
      physicalDevice.getSurfaceFormatsKHR(surface).value;

  std::vector<vk::PresentModeKHR> presentModes =
      physicalDevice.getSurfacePresentModesKHR(surface).value;

  format = chooseSurfaceFormat(formats);

  vk::PresentModeKHR presentMode = choosePresentMode(presentModes);


  extent = chooseExtent(width, height, capabilities);

  imageCount = capabilities.minImageCount + 1;
  if (capabilities.maxImageCount > 0 &&
      imageCount > capabilities.maxImageCount) {
    imageCount = capabilities.maxImageCount;
  }

  /*
  * VULKAN_HPP_CONSTEXPR SwapchainCreateInfoKHR(
  VULKAN_HPP_NAMESPACE::SwapchainCreateFlagsKHR flags_         = {},
  VULKAN_HPP_NAMESPACE::SurfaceKHR              surface_       = {},
  uint32_t                                      minImageCount_ = {},
  VULKAN_HPP_NAMESPACE::Format                  imageFormat_   =
  VULKAN_HPP_NAMESPACE::Format::eUndefined, VULKAN_HPP_NAMESPACE::ColorSpaceKHR
  imageColorSpace_  = VULKAN_HPP_NAMESPACE::ColorSpaceKHR::eSrgbNonlinear,
  VULKAN_HPP_NAMESPACE::Extent2D        imageExtent_      = {},
  uint32_t                              imageArrayLayers_ = {},
  VULKAN_HPP_NAMESPACE::ImageUsageFlags imageUsage_       = {},
  VULKAN_HPP_NAMESPACE::SharingMode     imageSharingMode_ =
  VULKAN_HPP_NAMESPACE::SharingMode::eExclusive, uint32_t queueFamilyIndexCount_
  = {}, const uint32_t *                      pQueueFamilyIndices_   = {},
  VULKAN_HPP_NAMESPACE::SurfaceTransformFlagBitsKHR preTransform_ =
  VULKAN_HPP_NAMESPACE::SurfaceTransformFlagBitsKHR::eIdentity,
  VULKAN_HPP_NAMESPACE::CompositeAlphaFlagBitsKHR compositeAlpha_ =
  VULKAN_HPP_NAMESPACE::CompositeAlphaFlagBitsKHR::eOpaque,
  VULKAN_HPP_NAMESPACE::PresentModeKHR presentMode_  =
  VULKAN_HPP_NAMESPACE::PresentModeKHR::eImmediate, VULKAN_HPP_NAMESPACE::Bool32
  clipped_      = {}, VULKAN_HPP_NAMESPACE::SwapchainKHR   oldSwapchain_ = {} )
  VULKAN_HPP_NOEXCEPT
  */
  vk::SwapchainCreateInfoKHR createInfo = vk::SwapchainCreateInfoKHR(
      vk::SwapchainCreateFlagsKHR(), surface, imageCount, format.format,
      format.colorSpace, extent, 1,
      vk::ImageUsageFlagBits::eColorAttachment |
          vk::ImageUsageFlagBits::eTransferDst);

  createInfo.preTransform = capabilities.currentTransform;
  createInfo.presentMode = presentMode;
  createInfo.clipped = VK_TRUE;

  createInfo.oldSwapchain = vk::SwapchainKHR(nullptr);

  auto res = logicalDevice.createSwapchainKHR(createInfo);
  if (res.result == vk::Result::eSuccess) {
    chain = res.value;
    chainGarbageQueue.push_back(
        [this](vk::Device device) { device.destroySwapchainKHR(chain); });
  } else {
    throw std::runtime_error("Failed to create SwapChain.");
  }
}

void Swapchain::build(vk::RenderPass renderPass) {
  std::vector<vk::Image> images =
      logicalDevice.getSwapchainImagesKHR(chain).value;

  for (uint32_t i = 0; i < images.size(); ++i) {
    frames.push_back(Frame(images[i], logicalDevice, format.format));
  }

  createFrameBuffers(renderPass);
}

void Swapchain::cleanUp() {

  while (chainGarbageQueue.size() > 0) {
    chainGarbageQueue.back()(logicalDevice);
    chainGarbageQueue.pop_back();
  }
}

vk::Extent2D Swapchain::chooseExtent(uint32_t width, uint32_t height,
                                     vk::SurfaceCapabilitiesKHR capabilities) {
  if (capabilities.currentExtent.width != UINT32_MAX) {
    return capabilities.currentExtent;
  }
  vk::Extent2D extent;

  extent.width = std::min(capabilities.maxImageExtent.width,
                          std::max(capabilities.minImageExtent.width, width));

  extent.height =
      std::min(capabilities.maxImageExtent.height,
               std::max(capabilities.minImageExtent.height, height));
  return extent;
}

vk::PresentModeKHR
Swapchain::choosePresentMode(std::vector<vk::PresentModeKHR> presentModes) {
  for (vk::PresentModeKHR mode : presentModes) {
    if (mode == vk::PresentModeKHR::eMailbox /*mode ==
        vk::PresentModeKHR::eImmediate*/) {
      Logger::log(Logger::LogLevel::DEBUG, "Present Mode: MailBox/Immediate");
      return mode;
    }
  }

  Logger::log(Logger::LogLevel::DEBUG, "Present Mode: FIFO");
  return vk::PresentModeKHR::eFifo;
}

vk::SurfaceFormatKHR
Swapchain::chooseSurfaceFormat(std::vector<vk::SurfaceFormatKHR> formats) {
  for (vk::SurfaceFormatKHR format : formats) {
    if (enableValidationLayers && chain == vk::SwapchainKHR{}) {
      Logger::log(Logger::LogLevel::DEBUG,
                  "Surface Format: " + vk::to_string(format.format) + " | " +
                      vk::to_string(format.colorSpace));
    }
    if (format.format == vk::Format::eB8G8R8A8Unorm &&
        format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
      return format;
    }
  }

  return formats[0];
}

void Swapchain::createFrameBuffers(vk::RenderPass renderPass) {
  frameBuffers.resize(imageCount);

  for (size_t i = 0; i < imageCount; i++) {
    std::array<vk::ImageView, 1> attachments = {frames[i].imageView};

    vk::FramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType = vk::StructureType::
        eFramebufferCreateInfo; // VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = renderPass;
    framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    framebufferInfo.pAttachments = attachments.data();
    framebufferInfo.width = extent.width;
    framebufferInfo.height = extent.height;
    framebufferInfo.layers = 1;

    auto res = logicalDevice.createFramebuffer(framebufferInfo);
    if (res.result != vk::Result::eSuccess) {
      throw std::runtime_error("failed to create framebuffer!");
    }
    frameBuffers[i] = res.value;
  }

  chainGarbageQueue.push_back(

      [this](vk::Device device) {
        for (size_t i = 0; i < frames.size(); i++) {
          device.destroyFramebuffer(frameBuffers[i]);
          device.destroyImageView(frames[i].imageView);
        }
        frames.clear();
      });
}
