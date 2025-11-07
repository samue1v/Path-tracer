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

#include <chrono>
#include <ctime>
#include <iomanip>

#include "Buffer.hpp"
#include "BufferOperator.hpp"
#include "Logger.hpp"
#include "ResourceImage.hpp"
#include "frame.hpp"
#include "objLoader.hpp"
#include "simpleMesh.hpp"
#include "swapchain.hpp"
#include "tracer.hpp"

#include "pipeline.hpp"
#include <vk_mem_alloc.h>

#include <atomic>
#include <map>

#define ENGINE_VERSION VK_MAKE_API_VERSION(0, 1, 0, 0)
class App;

/**
 * @brief Vulkan Engine Renderer
 */
class VulkanRender {
public:
  // Resources for the graphics part of the example
  struct Graphics {

    vk::DescriptorSetLayout descriptorSetLayout{VK_NULL_HANDLE};
    Pipeline pipeline;
    vk::PipelineLayout pipelineLayout{VK_NULL_HANDLE};
    vk::Semaphore acquireSemaphore, releaseSemaphore, copySemaphore;
    vk::Fence copyFinishFence;
    vk::Queue queue{VK_NULL_HANDLE};
    uint32_t queueIDX;
    std::vector<vk::CommandBuffer> commandBuffer{};
    vk::CommandBuffer acquireBuffer, releaseBuffer, copyBuffer;

    ResourceImage displayImg;

    vk::CommandPool commandPool{VK_NULL_HANDLE};

  } graphics;

  // Resources for the compute part of the example
  struct Compute {

    static constexpr uint32_t MAX_RAYS_PER_PIXEL = 15*15;
    uint32_t  rays_per_pixel = MAX_RAYS_PER_PIXEL; // 21 * 21;
    static constexpr uint32_t MAX_OBJECT_SIZE = 20;

    vk::Queue queue;
    uint32_t queueIDX;

    vk::CommandPool commandPool;
    vk::CommandBuffer commandBuffer;

    vk::DescriptorSetLayout descriptorSetLayout;
    vk::DescriptorSet descriptorSet;
    vk::DescriptorPool descriptorPool;
    vk::PushConstantRange pushConstantsRange;
    Pipeline pipeline;

    vk::CommandBuffer acquireBuffer, releaseBuffer;

    Buffer uniformBuffer;
    Buffer hitDataBuffer;
    Buffer RNGbuffer;
    Buffer spheresBuffer;
    Buffer planesBuffer;
    Buffer pixelDataBuffer;

    ResourceImage computeImg;
    Tracer::PushConstants constants;
    Tracer::camera cameraUniform;
    std::vector<Tracer::sphere> spheres;
    std::vector<Tracer::plane> planes;

    vk::Fence computeFence;
    vk::Semaphore computeFinishedSemaphore; // Framesinflight = 3
    vk::Semaphore acquireSemaphore, releaseSemaphore;

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
   * @brief Update Descriptors and push constants
   */
  void updateShaderData(vk::CommandBuffer cmdBuffer);

public:
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
   * @brief Build the swapchain images
   */
  void buildSwapchainImages();

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
   * @brief Handle the function swap from storage images
   */
  void swapComputePresentImages();

  /**
   * @brief Initialize buffer data if needed
   */
  void initializeBuffers();

  /**
   * @brief Initialize scene objects and camera
   */
  void initializeScene();

  /**
   * @brief Execute scene at index;
   * @params sceneIdx Index of scene
   */
  void execScene(uint32_t sceneIdx);

  /**
   * @brief Record compute command buffer
   * @param cmdBuffer Target command buffer and storage image to
   * record
   */
  void recordComputeCommandBuffer(vk::CommandBuffer cmdBuffer);

  /**
   * @brief Prepare images storage image and swapchain image for copy storage ->
   * swap
   * @params commandBuffer Command buffer for the operation
   * @params imageIdx Index of in flight frame swapchain image
   */
  void fromSwapchainToTransfer(vk::CommandBuffer commandBuffer, vk::Image img);

  /**
   * @brief Prepare swapchain image from transfer state to present transfer ->
   * present
   * @params commandBuffer Command buffer for the operation
   * @params Current in flight frame swapchain image
   */
  void fromTransferToSwapchain(vk::CommandBuffer, vk::Image img);

  /**
   * @brief Init IMGUI external lib
   */
  void initIMGUI();

  /**
   * @brief Updates camera
   * @params deltaPos glm::vec3 of delta
   * @params deltaAt glm::vec3 of delta
   */
  void updateCamera(glm::vec3 deltaPos, glm::vec3 deltaAt);

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
   * @brief Map of scenes
   */
  std::map<uint32_t, std::function<void()>> sceneMap;

  // IMGUI stuff
private:
  vk::DescriptorPool imguiDescriptorPool;

  int currentFPS;

  void showPerformanceMenu();

  void showMenu();

  // Shared stuff
public:
  std::atomic<int> *readIndex;
  std::atomic<int> *writeIndex;
  std::atomic<bool>*cameraMoved;
  std::array<Tracer::camera, 2> *buffer_camera;

  App *app;
};

#endif
