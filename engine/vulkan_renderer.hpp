#ifndef VULKAN_RENDERER
#define VULKAN_RENDERER

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <deque>
#include <functional>
#define VULKAN_HPP_NO_EXCEPTIONS
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

#include <glm/matrix.hpp>
#include <mutex>
#include <queue>
#include <vector>

#include "Buffer.hpp"
#include "Logger.hpp"
#include "ResourceImage.hpp"
#include "frame.hpp"
#include "objLoader.hpp"
#include "simpleMesh.hpp"
#include "swapchain.hpp"

#include "pipeline.hpp"
#include <vk_mem_alloc.h>

#define ENGINE_VERSION VK_MAKE_API_VERSION(0, 1, 0, 0)
class App;

struct uniform {
  alignas(16) glm::mat4 view;
  alignas(16) glm::mat4 proj;
};

struct alignas(16) hitData {
  glm::vec4 wo;
  glm::vec4 wi;
  glm::vec4 hit;
  glm::vec4 normal;
  glm::vec4 color;
  int depth;
  int pad[3];
};

struct alignas(16) data {
  int numSpheres;
  int numPlanes;
  int numLights;
  int numRays;
  int maxBounces;
  int pad[3];
};

/**
 * @brief Vulkan Engine Renderer
 */
class VulkanRender {
public:
  // Resources for the graphics part of the example
  struct Graphics {

    vk::DescriptorSetLayout descriptorSetLayout{VK_NULL_HANDLE};
    vk::DescriptorSet descriptorSetPreCompute{VK_NULL_HANDLE};

    vk::DescriptorSet descriptorSetPostCompute{VK_NULL_HANDLE};

    Pipeline pipeline;
    vk::PipelineLayout pipelineLayout{VK_NULL_HANDLE};
    vk::Semaphore semaphore{VK_NULL_HANDLE};
    vk::Queue queue{VK_NULL_HANDLE};
    std::vector<vk::CommandBuffer> commandBuffer{};

    vk::CommandPool commandPool{VK_NULL_HANDLE};

  } graphics;

  // Resources for the compute part of the example
  struct Compute {
    vk::Queue queue{VK_NULL_HANDLE};

    vk::CommandPool commandPool{VK_NULL_HANDLE};

    vk::CommandBuffer commandBuffer{VK_NULL_HANDLE};

    vk::DescriptorSetLayout descriptorSetLayout;
    std::array<vk::DescriptorSet, 2> descriptorSet;
    vk::DescriptorPool descriptorPool{VK_NULL_HANDLE};
    Pipeline pipeline;

    Buffer uniformBuffer;
    Buffer storageBuffer;
    std::array<ResourceImage, 2> storageImg;

    std::array<vk::Fence, 2> computeFences;
    vk::Semaphore computeFinishedSemaphore;
    uint32_t currentComputeBuffer = 0;

    static constexpr uint32_t rays_per_pixel = 1;
  } compute;
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
   * @brief Create image resources
   */
  void createImageResources();

  /**
   * @brief Create buffer resources
   */
  void createBufferResources();

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
   * @brief Create descriptors sets layouts
   */
  void createDescriptorSetLayout();

  /**
   * @brief Create pipeline
   */
  void createPipeline();

  /**
   * @brief Record draw command in the command buffer
   */
  void recordGraphicCommandBuffer(vk::CommandBuffer commandBuffer,
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
   * @brief Create descriptor sets
   */
  void createDescriptorSets();

  /**
   * @brief Create descriptor pool
   */
  void createDescriptorPool();

  /**
   * @brief Find
   */
  vk::Format findSupportedFormat(const std::vector<vk::Format> &candidates,
                                 vk::ImageTiling tiling,
                                 vk::FormatFeatureFlagBits features);

  /**
   * @brief Check if given format has a stencil component
   * @param format Format to be evaluated
   */
  bool hasStencilComponent(vk::Format format);

  /**
   * @brief Copy the current storage image to swapchain image
   * @param commandBufferIndex index of target command buffer and storage image to record
   */
  void copyComputeToSwapchain(uint32_t commandBufferIndex);

  /**
   * @brief Record compute command buffer
   * @param computeIndex index of target command buffer and storage image to record
   */
  void recordComputeCommandBuffer(uint32_t computeIndex);

  /**
   * @brief Init IMGUI external lib
   */
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
  VmaAllocator allocator;

  /**
   * @brief Loader for obj files
   */
  OBJLoader objLoader;

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

  /**
   * @brief Scene ViewProj matrix
   */
  uniform proj_view_uniforms;

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
