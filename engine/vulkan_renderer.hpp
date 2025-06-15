#ifndef VULKAN_RENDERER
#define VULKAN_RENDERER

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <deque>
#include <functional>
#define VULKAN_HPP_NO_EXCEPTIONS
#include <unordered_map>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <glm/matrix.hpp>
#include <iostream>
#include <mutex>
#include <optional>
#include <queue>
#include <random>
#include <thread>
#include <vector>

#include "allocator.hpp"
#include "frame.hpp"
#include "objLoader.hpp"
#include "pipelineManager.hpp"
#include "shared_structs.hpp"
#include "simpleMesh.hpp"
#include "swapchain.hpp"

#define ENGINE_VERSION VK_MAKE_API_VERSION(0, 1, 0, 0)
class App;

struct MV {
  alignas(16) glm::mat4 view;
  alignas(16) glm::mat4 proj;
};

/**
 * @brief Vulkan Engine Renderer
 */
class VulkanRender {
public:
  /**
   * @brief Construct new Engine object
   *
   * @param window glfw window to render to
   * @param appName application name
   */
  VulkanRender(GLFWwindow *window = nullptr, const char *appName = "Renderer");

  /**
   * @brief Initialize vulkan and stuff
   */
  void init();

  /**
   * @brief Load new mesh into engine
   * @params srcPath Source path to obj file
   * @params name Mesh name
   */
  void loadMesh(const std::string &srcPath, const std::string &name);

  /**
   * @brief Destroy the engine object
   */
  ~VulkanRender();

  /**
   * @brief Draws current frame
   */
  void drawFrame();

  /**
   * @brief Wait for logical device go IDLE
   */
  void waitIdle();

  /**
   * @brief Switch frameBufferResize 0-1
   */
  void switchResized();

  /**
   * @brief Sets current FPS
   */
  void setCurrentFps(int _fps);

  /**
   * @brief Update engine state
   */
  void updateState();

  /**
   * @brief Update MVP matrix
   */
  void updateMVP(glm::vec3 pos, glm::vec3 at);

  /**
   * @brief Uploads mesh to hull
   */
  void uploadHullDraw(std::string meshName);

public:
  std::queue<std::function<void()>> *renderCommands;
  std::mutex *renderQueueMutex;

  std::queue<std::function<void()>> mainCommands;
  std::mutex mainQueueMutex;

private:
  /**
   * @brief Instance builder
   */
  void createInstance();

  /**
   * @brief Physical device builder
   */
  void createPhysicalDevice();

  /**
   * @brief Tells if a physical device is suitable for the render needs
   * @param vk::PhysicalDevice Physical device to be tested
   * @return Physical device is suitable
   */
  bool isSuitable(const vk::PhysicalDevice);

  /**
   * @brief Creates the debug messenger
   */
  void createDebugMessenger();

  /**
   * @brief Debug callback for vulkan
   * @param messageSeverity Debug message Severity
   * @param messageType Type of message
   * @param pCallbackData Pointer to Callback function
   * @param pUserData Pointer to user data
   */
  VKAPI_ATTR VkBool32 VKAPI_CALL static debugCallback(
      VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
      VkDebugUtilsMessageTypeFlagsEXT messageType,
      const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
      void *pUserData);

  /**
   * @brief Find index of queue of type /p queueType from /p physicalDevice
   * @param physicalDevice Physical Device
   * @param queueType Queue Type
   * @return uint32_t Index of family
   */
  uint32_t findQueueFamilyIndex(vk::QueueFlags queueType);

  /**
   * @brief Create a logical device object
   */
  void createLogicalDevice();

  /**
   * @brief Create Surface
   */
  void createSurface();

  /**
   * @brief Creates the swapchain
   */
  void createSwapChain();

  /**
   * @brief Creates the command pool
   */
  void createCommandPool();

  /**
   * @brief Builds the render pass
   */
  void buildRenderPass();

  /**
   * @brief Creates and initializes vma allocator wrapper
   */
  void createAllocator();

  /**
   * @brief Create command buffer
   */
  void createCommandBuffers();

  /**
   * @brief Record draw command in the command buffer
   */
  void recordCommandBuffer(vk::CommandBuffer commandBuffer,
                           uint32_t imageIndex);

  /**
   * @brief Create uniform buffers
   */
  void createUniformBuffers();

  /**
   * @brief Create Semaphores and Fences
   */
  void createSyncObjects();

  /**
   * @brief Recreates the swap chain
   */
  void recreateSwapChain();

  void createDescriptorSets();

  void createDepthResources();

  vk::Format findSupportedFormat(const std::vector<vk::Format> &candidates,
                                 vk::ImageTiling tiling,
                                 vk::FormatFeatureFlagBits features);

  vk::Format findDepthFormat();

  bool hasStencilComponent(vk::Format format);

  void initIMGUI();

private:
  /**
   * @brief GLFW window pointer
   */
  GLFWwindow *window;

  /**
   * @brief Vulkan main Instance
   */
  vk::Instance instance;

  /**
   * @brief Physical device(GPU)
   */
  vk::PhysicalDevice physicalDevice;

  /**
   * @brief Logical device
   */
  vk::Device logicalDevice;

  /**
   * @brief Graphics queue
   */
  vk::Queue graphicsQueue;

  /**
   * @brief Presentation queue
   */
  vk::Queue presentQueue;

  /**
   * @brief Surface
   */
  vk::SurfaceKHR surface;

  /**
   * @brief Swapchain
   */
  Swapchain chain;

  /**
   * @brief Current render pass
   */
  vk::RenderPass renderPass;

  /**
   * @brief Command Pool
   */
  vk::CommandPool cmdPool;

  /**
   * @brief Vector of descriptor sets
   */
  std::vector<vk::DescriptorSet> descriptorSets;

  /**
   * @brief Command Buffers
   */
  std::vector<vk::CommandBuffer> cmdBuffers;

  /**
   * @brief Semaphore that signals when swapchain image is avaliable
   */
  std::vector<vk::Semaphore> imageAvailableSemaphores;

  /**
   * @brief Semaphore that signals when swapchain image render is finished;
   */
  std::vector<vk::Semaphore> renderFinishedSemaphores;

  /**
   * @brief Fence for CPU-GPU synchronization
   */
  std::vector<vk::Fence> inFlightFences;

  /**
   * @brief Queue of instance related garbage to be deleted
   */
  std::deque<std::function<void(vk::Instance)>> instanceGarbageQueue;

  /**
   * @brief Queue of device related immutable garbage to be deleted
   */
  std::deque<std::function<void(vk::Device)>> deviceGlobalGarbageQueue;

  /**
   * @brief Queue of device related mutable garbage to be deleted
   */
  // std::deque<std::function<void(vk::Device)>> deviceLocalGarbageQueue;

  /**
   * @brief Debug messenger
   */
  vk::DebugUtilsMessengerEXT debugMessenger;

  /**
   * @brief Application name
   */
  const char *appName;

  /**
   * @brief Instance extensions for future reference
   */
  std::vector<vk::ExtensionProperties> instanceSupportedExtensions;

  /**
   * @brief Wrapper for vma allocation.
   */
  Allocator allocatorWrapper;

  /**
   * @brief Loader for obj files
   */
  OBJLoader objLoader;

  /**
   * @brief Pipeline Manager
   */
  PipelineManager pipeManager;

  /**
   * @brief Max frames in flight
   */
  int framesInFlight;

  /**
   * @brief Boolean if window is resized
   */
  bool frameBufferResized;

  /**
   * @brief Current frame index
   */
  uint32_t currentFrame;

  MV testMV{};

  // IMGUI stuff
private:
  vk::DescriptorPool imguiDescriptorPool;

  int currentFPS;

  void showPerformanceMenu();

  void showMenu();


  // Shared stuff
public:
  App *app;
};

#endif
