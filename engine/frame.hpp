#ifndef FRAME_HPP
#define FRAME_HPP

#define VULKAN_HPP_NO_EXCEPTIONS
#include <vulkan/vulkan.hpp>

/**
 * @brief Hold rendering states
 */
class Frame {
public:
  Frame() = default;
  /**
   * @brief
   * @param image swapchain image to render to
   * @param logicalDevice vulkan device
   * @param swapchainFormat swapchain image format
   * @param deletionQueue logical device deletion queue
   */
  Frame(vk::Image image, vk::Device logicalDevice, vk::Format swapchainFormat);

private:
  /**
   * @brief Creates the swapchain view
   */
  void createImageView(vk::Device logicalDevice);

public:
  /**
   * @brief Raw swapchain image
   */
  vk::Image image;

  /**
   * @brief View of the raw swapchain image
   */
  vk::ImageView imageView;

  /**
   * @brief Image format
   */
  vk::Format format;

  /**
   * @brief Image Layout
   */
  vk::ImageLayout layout;
};

#endif
