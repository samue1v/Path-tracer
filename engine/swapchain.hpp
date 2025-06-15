#ifndef SWAPCHAIN_HPP
#define SWAPCHAIN_HPP
#define VULKAN_HPP_NO_EXCEPTIONS
#include "../config/config.hpp"
#include "frame.hpp"
#include "FB_attachment.hpp"
#include <deque>
#include <functional>
#include <vulkan/vulkan.hpp>

class Swapchain {
public:
  /**
   * @brief Construct a new Swapchain object
   *
   * @param logicalDevice vulkan device
   * @param physicalDevice physical device
   * @param surface the window surface to present to
   * @param width requested swapchain width
   * @param height requested swapchain height
   */
  void create(vk::Device logicalDevice, vk::PhysicalDevice physicalDevice,
              vk::SurfaceKHR surface, uint32_t width, uint32_t height);

  /**
   * @brief Populates the new swapchain
   */
  void build(vk::RenderPass renderPass);

  /**
   * @brief Create frameBuffers
   */
  void createFrameBuffers(vk::RenderPass renderPass);

  void createAttachment(VmaAllocator & allocator);

  /**
   * @brief Cleans the swap chain for recreation
   */
  void cleanUp();

  /**
   * @brief the number of images
   *
   */
  uint32_t imageCount;

  /**
   * @brief The underlying swapchain resource
   *
   */
  vk::SwapchainKHR chain;

  /**
   * @brief image format
   *
   */
  vk::SurfaceFormatKHR format;

  /**
   * @brief image size
   *
   */
  vk::Extent2D extent;

  /**
   * @brief Frames used for rendering
   *
   */
  std::vector<Frame> frames;


  /**
   * @brief Frame buffer vector
   */
  std::vector<vk::Framebuffer> frameBuffers;

  /**
   * @brief Logical Device
   */
  vk::Device logicalDevice;
  

  FB_Attachment depthAttachment;
private:
  /**
   * @brief Choose an extent, working within the given constraints
   *
   * @param width requested width
   * @param height requested height
   * @param capabilities surface capability support
   * @return vk::Extent2D the chosen extent
   */
  vk::Extent2D chooseExtent(uint32_t width, uint32_t height,
                            vk::SurfaceCapabilitiesKHR capabilities);

  /**
   * @brief Choose a present mode
   *
   * @param presentModes available present modes
   * @return vk::PresentModeKHR the chosen present mode
   */
  vk::PresentModeKHR
  choosePresentMode(std::vector<vk::PresentModeKHR> presentModes);

  /**
   * @brief Choose a surface format
   *
   * @param formats supported formats to choose from
   * @return vk::SurfaceFormatKHR the chosen format
   */
  vk::SurfaceFormatKHR
  chooseSurfaceFormat(std::vector<vk::SurfaceFormatKHR> formats);

private:
  /**
   * @brief SwapChain garbage queue
   */
  std::deque<std::function<void(vk::Device)>> chainGarbageQueue;
};

#endif
