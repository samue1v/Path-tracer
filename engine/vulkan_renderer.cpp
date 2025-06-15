#include "vulkan_renderer.hpp"
#include "../config/config.hpp"
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "app.hpp"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_vulkan.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

VulkanRender::VulkanRender(GLFWwindow *window, const char *appName)
    : window(window), appName(appName), framesInFlight(3), currentFrame(0),
      frameBufferResized(0) {}

void VulkanRender::init() {
  createInstance();

  createSurface();

  createPhysicalDevice();

  createDebugMessenger();

  createLogicalDevice();

  createSyncObjects();

  createSwapChain();

  createAllocator();

  createCommandPool();

  buildRenderPass();

  chain.createAttachment(allocatorWrapper.allocator);

  createDepthResources();

  chain.build(renderPass);

  createUniformBuffers();

  pipeManager = PipelineManager(logicalDevice, renderPass);

  pipeManager.createDescriptorSetLayout();

  pipeManager.createDescriptorPool(framesInFlight);

  createDescriptorSets();

  createCommandBuffers();

  initIMGUI();
}

VulkanRender::~VulkanRender() {

  ImGui_ImplVulkan_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
  logicalDevice.destroyDescriptorPool(imguiDescriptorPool);

  pipeManager.cleanUp();

  chain.cleanUp();

  allocatorWrapper.cleanUp();

  while (deviceGlobalGarbageQueue.size() > 0) {
    deviceGlobalGarbageQueue.back()(logicalDevice);
    deviceGlobalGarbageQueue.pop_back();
  }

  while (instanceGarbageQueue.size() > 0) {
    instanceGarbageQueue.back()(instance);
    instanceGarbageQueue.pop_back();
  }
}

void VulkanRender::loadMesh(const std::string &srcPath,
                            const std::string &name) {}

void VulkanRender::createAllocator() {
  VmaAllocator vma_allocator;
  VmaAllocatorCreateInfo createInfo{};
  createInfo.physicalDevice = physicalDevice;
  createInfo.device = logicalDevice;
  createInfo.pHeapSizeLimit = nullptr;
  createInfo.pVulkanFunctions = nullptr;
  createInfo.instance = instance;
  createInfo.pTypeExternalMemoryHandleTypes = nullptr;

  VmaVulkanFunctions vkFunctions{};
  vkFunctions.vkGetInstanceProcAddr = &vkGetInstanceProcAddr;
  vkFunctions.vkGetDeviceProcAddr = &vkGetDeviceProcAddr;

  createInfo.pVulkanFunctions = &vkFunctions;

  if (vmaCreateAllocator(&createInfo, &vma_allocator) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create vma allocator");
  }

  allocatorWrapper.initialize(logicalDevice, vma_allocator, 1024 * 1024,
                              1024 * 1024);
}

void VulkanRender::createUniformBuffers() {
  allocatorWrapper.allocUniformBuffers(sizeof(MV), framesInFlight);
}

void VulkanRender::updateMVP(glm::vec3 pos, glm::vec3 at) {}

void VulkanRender::drawFrame() {
  logicalDevice.waitForFences(1, &inFlightFences[currentFrame], vk::True,
                              UINT64_MAX);
  auto resImg = logicalDevice.acquireNextImageKHR(
      chain.chain, UINT64_MAX, imageAvailableSemaphores[currentFrame], nullptr);
  if (resImg.result == vk::Result::eErrorOutOfDateKHR) {
    recreateSwapChain();
    return;
  } else if (resImg.result != vk::Result::eSuccess &&
             resImg.result != vk::Result::eSuboptimalKHR) {
    throw std::runtime_error("Failed to acquire swap chain image.");
  }

  logicalDevice.resetFences(1, &inFlightFences[currentFrame]);
  uint32_t imageIdx = resImg.value;

  ImGui_ImplVulkan_NewFrame(); // Vulkan state updates
  ImGui_ImplGlfw_NewFrame();   // GLFW input processing
  ImGui::NewFrame();           // Starts a new Dear ImGui frame

  showPerformanceMenu();
  showMenu();

  cmdBuffers[currentFrame].reset();
  recordCommandBuffer(cmdBuffers[currentFrame], imageIdx);

  vk::Semaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
  vk::Semaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
  vk::PipelineStageFlags waitStages[] = {
      vk::PipelineStageFlagBits::eColorAttachmentOutput};

  vk::SubmitInfo submitInfo{};
  submitInfo.sType = vk::StructureType::eSubmitInfo;
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = waitSemaphores;
  submitInfo.pWaitDstStageMask = waitStages;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &cmdBuffers[currentFrame];
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = signalSemaphores;

  auto resSubmit =
      graphicsQueue.submit(1, &submitInfo, inFlightFences[currentFrame]);
  if (resSubmit != vk::Result::eSuccess) {
    throw std::runtime_error("Failed to submit draw command buffer");
  }

  vk::PresentInfoKHR presentInfo{};
  presentInfo.sType = vk::StructureType::ePresentInfoKHR;
  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = signalSemaphores;

  vk::SwapchainKHR swapChains[] = {chain.chain};

  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = swapChains;
  presentInfo.pImageIndices = &imageIdx;
  presentInfo.pResults = nullptr;

  auto resPres = presentQueue.presentKHR(&presentInfo);

  if (resPres == vk::Result::eErrorOutOfDateKHR ||
      resPres == vk::Result::eSuboptimalKHR || frameBufferResized) {
    switchResized();
    recreateSwapChain();
  } else if (resPres != vk::Result::eSuccess &&
             resPres != vk::Result::eSuboptimalKHR) {
    throw std::runtime_error("Failed to present swap chain image.");
  }
  currentFrame = (currentFrame + 1) % framesInFlight;
}

void VulkanRender::createDescriptorSets() {
  std::vector<vk::DescriptorSetLayout> layouts(
      framesInFlight, pipeManager.getDescriptorSetLayout());
  vk::DescriptorSetAllocateInfo allocInfo{};
  allocInfo.sType = vk::StructureType::eDescriptorSetAllocateInfo;
  allocInfo.descriptorPool = pipeManager.getDescriptorPool();
  allocInfo.descriptorSetCount = static_cast<uint32_t>(framesInFlight);
  allocInfo.pSetLayouts = layouts.data();

  descriptorSets.resize(framesInFlight);

  auto resAlloc = logicalDevice.allocateDescriptorSets(allocInfo);
  if (resAlloc.result != vk::Result::eSuccess) {
    throw std::runtime_error("Failed to allocate descriptor sets");
  }
  descriptorSets = resAlloc.value;

  for (size_t i = 0; i < framesInFlight; i++) {
    vk::DescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = allocatorWrapper.uniformBuffers[i];
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(MV);

    vk::WriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType = vk::StructureType::eWriteDescriptorSet;
    descriptorWrite.dstSet = descriptorSets[i];
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = vk::DescriptorType::eUniformBuffer;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pBufferInfo = &bufferInfo;

    logicalDevice.updateDescriptorSets(1, &descriptorWrite, 0, nullptr);
  }
}

void VulkanRender::createInstance() {

  // Extensions
  instanceSupportedExtensions =
      vk::enumerateInstanceExtensionProperties().value;

  vk::ApplicationInfo appInfo =
      vk::ApplicationInfo(this->appName, vk::enumerateInstanceVersion().value,
                          this->appName, ENGINE_VERSION, ENGINE_VERSION);
  uint32_t glfwExtensionCount = 0;
  const char **glfwExtensions;
  glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
  uint32_t enabledExtensionCount = glfwExtensionCount;

  if (enableValidationLayers) {
    enabledExtensionCount++;
  }

  const char **ppEnabledExtensionNames =
      (const char **)malloc(enabledExtensionCount * sizeof(char *));

  for (int i = 0; i < glfwExtensionCount; ++i) {
    ppEnabledExtensionNames[i] = glfwExtensions[i];
  }
  if (enableValidationLayers) {
    ppEnabledExtensionNames[enabledExtensionCount - 1] =
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
  }

  bool found;
  for (int i = 0; i < enabledExtensionCount; ++i) {
    const char *extension = ppEnabledExtensionNames[i];
    found = false;
    for (vk::ExtensionProperties supportedExtension :
         instanceSupportedExtensions) {
      if (enableValidationLayers) {
        std::cout << "LAYER:" << supportedExtension.extensionName << std::endl;
      }
      if (strcmp(extension, supportedExtension.extensionName) == 0) {
        found = true;
        break;
      }
    }
    if (!found) {
      throw std::runtime_error(
          "Required extensions are not avaliable for this instance.");
    }
  }

  // Layers

  std::vector<vk::LayerProperties> supportedLayers =
      vk::enumerateInstanceLayerProperties().value;

  uint32_t enabledLayerCount = 0;
  if (enableValidationLayers) {
    enabledLayerCount++;
  }

  const char **ppEnabledLayerNames = nullptr;

  if (enabledLayerCount > 0) {
    ppEnabledLayerNames =
        (const char **)malloc(enabledLayerCount * sizeof(char *));
  }

  if (enableValidationLayers) {
    ppEnabledLayerNames[0] = "VK_LAYER_KHRONOS_validation";
  }

  for (int i = 0; i < enabledLayerCount; ++i) {
    const char *layer = ppEnabledLayerNames[i];
    found = false;
    for (vk::LayerProperties supportedLayer : supportedLayers) {
      if (strcmp(layer, supportedLayer.layerName) == 0) {
        found = true;
        break;
      }
    }
    if (!found) {
      throw std::runtime_error("Requested Layers are not avaliable.");
    }
  }

  vk::InstanceCreateInfo createInfo = vk::InstanceCreateInfo(
      vk::InstanceCreateFlags(), &appInfo, enabledLayerCount,
      ppEnabledLayerNames, enabledExtensionCount, ppEnabledExtensionNames);

  VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
  if (enableValidationLayers) {

    debugCreateInfo.sType =
        VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debugCreateInfo.messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debugCreateInfo.messageType =
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debugCreateInfo.pfnUserCallback = debugCallback;

    createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT *)&debugCreateInfo;
  }

  vk::ResultValue<vk::Instance> instanceAttempt =
      vk::createInstance(createInfo);
  if (instanceAttempt.result != vk::Result::eSuccess) {
    throw std::runtime_error("ck::createInstance failed.");
  }

  instance = instanceAttempt.value;

  instanceGarbageQueue.push_back(
      [](vk::Instance instance) { instance.destroy(); });

  free(ppEnabledExtensionNames);
  if (ppEnabledLayerNames) {
    free(ppEnabledLayerNames);
  }
}

void VulkanRender::createPhysicalDevice() {

  std::vector<vk::PhysicalDevice> availableDevices =
      instance.enumeratePhysicalDevices().value;
  VkPhysicalDeviceProperties prop;
  for (vk::PhysicalDevice device : availableDevices) {
    if (enableValidationLayers) {
      vkGetPhysicalDeviceProperties(device, &prop);
      std::cout << "DEVICE: " << prop.deviceName << std::endl;
    }
    if (isSuitable(device)) {
      physicalDevice = device;
      break;
    }
  }
  if (!physicalDevice) {
    throw std::runtime_error("Unable to find suitable gpu");
  }
}

bool VulkanRender::isSuitable(const vk::PhysicalDevice device) {

  /**
   * @brief Check if a array of requested extensions is supported by the
   * physical device
   *
   * @param device Physical device to be tested
   * @param ppRequestedExtensions Array of requested extensions names
   * @params requestedExtensionCount Number of requested extensions
   *
   * @details
   * \p ppRequestedExtension and \p requestedExtensionCount should be replaced
   * lately by a std::vector<const char *>
   */
  auto supports = [this](vk::PhysicalDevice device,
                         const char **ppRequestedExtensions,
                         const uint32_t requestedExtensionCount) {
    std::vector<vk::ExtensionProperties> extensions =
        device.enumerateDeviceExtensionProperties().value;
    for (uint32_t i = 0; i < requestedExtensionCount; ++i) {
      bool supported = false;

      for (vk::ExtensionProperties &extension :
           /*supportedExtensions*/ extensions) {
        std::string name = extension.extensionName;

        if (enableValidationLayers) {
          std::cout << "EXT: " << name << std::endl;
        }
        if (!name.compare(ppRequestedExtensions[i])) {
          supported = true;
        }
      }
      if (!supported) {
        return false;
      }
    }
    return true;
  };

  const char *ppRequestedExtension = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
  uint32_t requestedExtensionCount = 1;

  if (!supports(device, &ppRequestedExtension, 1)) {
    return false;
  }
  return true;
}

void VulkanRender::createDebugMessenger() {
  if (!enableValidationLayers) {
    return;
  }

  VkDebugUtilsMessengerCreateInfoEXT createInfo;
  createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  createInfo.pfnUserCallback = debugCallback;

  VkDebugUtilsMessengerEXT handle; // = debugMessenger;

  auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
      instance, "vkCreateDebugUtilsMessengerEXT");
  if (func != nullptr) {
    func(instance, &createInfo, nullptr, &handle);
  } else {
    throw std::runtime_error("Unable to create debug messenger.");
  }

  debugMessenger = vk::DebugUtilsMessengerEXT(handle);

  instanceGarbageQueue.push_back([this, handle](vk::Instance instance) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
        instance, "vkDestroyDebugUtilsMessengerEXT");

    if (func != nullptr) {

      func(instance, handle, nullptr);
    }
  });
}

VKAPI_ATTR VkBool32 VKAPI_CALL VulkanRender::debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
    void *pUserData) {
  std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

  return VK_FALSE;
}

uint32_t VulkanRender::findQueueFamilyIndex(vk::QueueFlags queueType) {

  std::vector<vk::QueueFamilyProperties> queueFamilies =
      physicalDevice.getQueueFamilyProperties();

  for (uint32_t i = 0; i < queueFamilies.size(); ++i) {

    vk::QueueFamilyProperties queueFamily = queueFamilies[i];

    bool canPresent = true;
    if (surface) {
      if (physicalDevice.getSurfaceSupportKHR(i, surface).result !=
          vk::Result::eSuccess) {
        canPresent = false;
      }
    }

    bool supported = false;
    if (queueFamily.queueFlags & queueType) {
      supported = true;
    }

    if (supported && canPresent) {
      return i;
    }
  }

  return UINT32_MAX;
}

void VulkanRender::createLogicalDevice() {

  uint32_t graphicsIndex = findQueueFamilyIndex(vk::QueueFlagBits::eGraphics);
  uint32_t presentIndex = graphicsIndex;

  float queuePriority = 1.0f;

  /*
  * VULKAN_HPP_CONSTEXPR DeviceQueueCreateInfo(
          VULKAN_HPP_NAMESPACE::DeviceQueueCreateFlags flags_	= {},
  uint32_t                          queueFamilyIndex_ = {},
  uint32_t                          queueCount_       = {},
  const float * pQueuePriorities_ = {} ) VULKAN_HPP_NOEXCEPT
  */
  vk::DeviceQueueCreateInfo queueCreateInfo = vk::DeviceQueueCreateInfo(
      vk::DeviceQueueCreateFlags(), graphicsIndex, 1, &queuePriority);

  VkDeviceQueueCreateInfo queueCreateHandle = queueCreateInfo;

  /*
   * Device features must be requested before the device is abstracted,
   * so that we only pay for what we need.
   */
  vk::PhysicalDeviceFeatures deviceFeatures = vk::PhysicalDeviceFeatures();

  VkPhysicalDeviceFeatures physicalDeviceFeaturesHandle = deviceFeatures;
  physicalDeviceFeaturesHandle.fillModeNonSolid = VK_TRUE;
  physicalDeviceFeaturesHandle.wideLines = VK_TRUE;

  std::vector<const char *> deviceExtensions;
  deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

  VkDeviceCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  createInfo.pNext = nullptr;
  createInfo.queueCreateInfoCount = 1;
  createInfo.pQueueCreateInfos = &queueCreateHandle;
  createInfo.enabledExtensionCount = deviceExtensions.size();
  createInfo.ppEnabledExtensionNames = deviceExtensions.data();
  createInfo.pEnabledFeatures = &physicalDeviceFeaturesHandle;

  VkDevice deviceHandle;

  if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &deviceHandle) !=
      VK_SUCCESS) {
    throw std::runtime_error("Could not create logical device");
  }

  logicalDevice = vk::Device(deviceHandle);

  graphicsQueue = logicalDevice.getQueue(graphicsIndex, 0);
  presentQueue = logicalDevice.getQueue(presentIndex, 0);
  deviceGlobalGarbageQueue.push_back(
      [](vk::Device device) { device.destroy(); });
}

void VulkanRender::createSurface() {
  VkSurfaceKHR surfaceHandle;
  glfwCreateWindowSurface(instance, window, nullptr, &surfaceHandle);
  surface = vk::SurfaceKHR(surfaceHandle);

  instanceGarbageQueue.push_back(
      [this](vk::Instance instance) { instance.destroySurfaceKHR(surface); });
}

void VulkanRender::createSwapChain() {
  int width, height;
  glfwGetFramebufferSize(window, &width, &height);
  chain.create(logicalDevice, physicalDevice, surface, width, height);
}

// GLITCHED DO NOT RESIZE!!!!!!!!!
void VulkanRender::recreateSwapChain() {
  int width = 0, height = 0;
  while (width == 0 || height == 0) {
    glfwGetFramebufferSize(window, &width, &height);
    glfwWaitEvents();
  }
  logicalDevice.waitIdle();
  chain.cleanUp();
  createSwapChain();

  createDepthResources();
  chain.build(renderPass);
}

void VulkanRender::buildRenderPass() {
  vk::AttachmentDescription colorAttachment{};
  colorAttachment.format = chain.format.format;
  colorAttachment.samples =
      vk::SampleCountFlagBits::e1; // VK_SAMPLE_COUNT_1_BIT;
  colorAttachment.loadOp =
      vk::AttachmentLoadOp::eClear; // VK_ATTACHMENT_LOAD_OP_CLEAR;
  colorAttachment.storeOp =
      vk::AttachmentStoreOp::eStore; // VK_ATTACHMENT_STORE_OP_STORE;
  colorAttachment.stencilLoadOp =
      vk::AttachmentLoadOp::eDontCare; // VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  colorAttachment.stencilStoreOp =
      vk::AttachmentStoreOp::eDontCare; // VK_ATTACHMENT_STORE_OP_DONT_CARE;
  colorAttachment.initialLayout =
      vk::ImageLayout::eUndefined; // VK_IMAGE_LAYOUT_UNDEFINED;
  colorAttachment.finalLayout =
      vk::ImageLayout::ePresentSrcKHR; // VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  vk::AttachmentReference colorAttachmentRef{};
  colorAttachmentRef.attachment = 0;
  colorAttachmentRef.layout = vk::ImageLayout::
      eColorAttachmentOptimal; // VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                               //
  vk::AttachmentDescription depthAttachment{};
  depthAttachment.format = findDepthFormat();
  depthAttachment.samples = vk::SampleCountFlagBits::e1;
  depthAttachment.loadOp = vk::AttachmentLoadOp::eClear;
  depthAttachment.storeOp = vk::AttachmentStoreOp::eStore; // eDontCare;
  depthAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
  depthAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
  depthAttachment.initialLayout = vk::ImageLayout::eUndefined;
  depthAttachment.finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

  vk::AttachmentReference depthAttachmentRef{};
  depthAttachmentRef.attachment = 1;
  depthAttachmentRef.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

  vk::SubpassDescription subpass{};
  subpass.pipelineBindPoint =
      vk::PipelineBindPoint::eGraphics; // VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colorAttachmentRef;
  subpass.pDepthStencilAttachment = &depthAttachmentRef;

  // VkSubpassDependency dependency{};
  vk::SubpassDependency dependency{};
  dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass = 0;
  dependency.srcStageMask =
      vk::PipelineStageFlagBits::eColorAttachmentOutput |
      vk::PipelineStageFlagBits::
          eEarlyFragmentTests; // eLateFragmentTests; //
                               // VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.srcAccessMask =
      vk::AccessFlagBits::eNone; // eDepthStencilAttachmentWrite; //
                                 // vk::AccessFlagBits::eNone;
  dependency.dstStageMask =
      vk::PipelineStageFlagBits::eColorAttachmentOutput |
      vk::PipelineStageFlagBits::
          eEarlyFragmentTests; // VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.dstAccessMask =
      vk::AccessFlagBits::eColorAttachmentWrite |
      vk::AccessFlagBits::
          eDepthStencilAttachmentWrite; // VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                                        //
  std::array<vk::AttachmentDescription, 2> attachments = {colorAttachment,
                                                          depthAttachment};

  vk::RenderPassCreateInfo renderPassInfo{};
  renderPassInfo.sType = vk::StructureType::
      eRenderPassCreateInfo; // VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
  renderPassInfo.pAttachments = attachments.data();
  renderPassInfo.subpassCount = 1;
  renderPassInfo.pSubpasses = &subpass;
  renderPassInfo.dependencyCount = 1;
  renderPassInfo.pDependencies = &dependency;

  auto res = logicalDevice.createRenderPass(renderPassInfo);
  if (res.result == vk::Result::eSuccess) {
    renderPass = res.value;

    deviceGlobalGarbageQueue.push_back(

        [this](vk::Device device) { device.destroyRenderPass(renderPass); });
  } else {
    throw std::runtime_error("failed to create render pass!");
  }
}

void VulkanRender::createCommandPool() {
  uint32_t graphicsFamilyIdx =
      findQueueFamilyIndex(vk::QueueFlagBits::eGraphics);

  vk::CommandPoolCreateInfo poolInfo{};
  poolInfo.sType = vk::StructureType::
      eCommandPoolCreateInfo; // VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.flags = vk::CommandPoolCreateFlagBits::
      eResetCommandBuffer; // VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  poolInfo.queueFamilyIndex = graphicsFamilyIdx;

  auto res = logicalDevice.createCommandPool(poolInfo);

  if (res.result != vk::Result::eSuccess) {
    throw std::runtime_error("failed to create graphics command pool!");
  }
  cmdPool = res.value;
  deviceGlobalGarbageQueue.push_back(
      [this](vk::Device device) { device.destroyCommandPool(cmdPool); });
}

void VulkanRender::createCommandBuffers() {
  cmdBuffers.resize(framesInFlight);
  vk::CommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = vk::StructureType::eCommandBufferAllocateInfo;
  allocInfo.commandPool = cmdPool;
  allocInfo.level = vk::CommandBufferLevel::ePrimary;
  allocInfo.commandBufferCount = static_cast<uint32_t>(cmdBuffers.size());

  auto res =
      logicalDevice.allocateCommandBuffers(&allocInfo, cmdBuffers.data());
  if (res != vk::Result::eSuccess) {
    throw std::runtime_error("Failed to allocate command buffer");
  }
}

void VulkanRender::recordCommandBuffer(vk::CommandBuffer commandBuffer,
                                       uint32_t imageIndex) {
  vk::CommandBufferBeginInfo beginInfo{};
  beginInfo.sType = vk::StructureType::eCommandBufferBeginInfo;
  beginInfo.pInheritanceInfo = nullptr;

  auto res = commandBuffer.begin(&beginInfo);
  if (res != vk::Result::eSuccess) {
    throw std::runtime_error("Failed to begin cmdBuffer");
  }

  vk::ClearColorValue clearValColor = {57.f / 255.f, 62.f / 255.f, 70.f / 255.f,
                                       1.f};
  std::array<vk::ClearValue, 2> clearValues{};
  vk::ClearValue clearValCol;
  clearValCol.setColor(clearValColor);

  vk::ClearValue clearValDepht;
  clearValDepht.setDepthStencil({1.0f, 0});
  clearValues[0] = clearValCol;
  clearValues[1] = clearValDepht;

  vk::RenderPassBeginInfo renderPassInfo{};
  renderPassInfo.sType = vk::StructureType::eRenderPassBeginInfo;
  renderPassInfo.renderPass = renderPass;
  renderPassInfo.framebuffer = chain.frameBuffers[imageIndex];
  renderPassInfo.renderArea.offset = vk::Offset2D(0, 0);
  renderPassInfo.renderArea.extent = chain.extent;
  renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
  ;
  renderPassInfo.pClearValues = clearValues.data();

  commandBuffer.beginRenderPass(&renderPassInfo, vk::SubpassContents::eInline);

  vk::Viewport viewport{};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = (float)chain.extent.width;
  viewport.height = (float)chain.extent.height;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  commandBuffer.setViewport(0, 1, &viewport);

  vk::Rect2D scissor{};
  scissor.offset = {{0, 0}};
  scissor.extent = chain.extent;
  commandBuffer.setScissor(0, 1, &scissor);

  vk::DeviceSize offsets[] = {0};
  vk::Buffer vertexBuffers[] = {allocatorWrapper.getVertexBuffer()};

  commandBuffer.bindVertexBuffers(0, 1, vertexBuffers, offsets);

  commandBuffer.bindIndexBuffer(allocatorWrapper.getIndexBuffer(), 0,
                                vk::IndexType::eUint32);

  // DRAW LOGIC AND CALLS SHOULD BE HERE

  ImGui::Render();
  ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(),
                                  static_cast<VkCommandBuffer>(commandBuffer));

  commandBuffer.endRenderPass();
  auto resEnd = commandBuffer.end();
  if (resEnd != vk::Result::eSuccess) {
    throw std::runtime_error("Failed to end command buffer");
  }
}

void VulkanRender::createDepthResources() {
  vk::Format depthFormat = findDepthFormat();
  chain.depthAttachment.createAttachment(depthFormat, chain.extent);
}

void VulkanRender::createSyncObjects() {

  imageAvailableSemaphores.resize(framesInFlight);
  renderFinishedSemaphores.resize(framesInFlight);
  inFlightFences.resize(framesInFlight);

  vk::SemaphoreCreateInfo semaphoreInfo{};
  semaphoreInfo.sType = vk::StructureType::eSemaphoreCreateInfo;

  vk::FenceCreateInfo fenceInfo{};
  fenceInfo.sType = vk::StructureType::eFenceCreateInfo;
  fenceInfo.flags = vk::FenceCreateFlagBits::eSignaled;

  for (int i = 0; i < framesInFlight; i++) {
    auto resImgAvl = logicalDevice.createSemaphore(semaphoreInfo);
    auto resRendFin = logicalDevice.createSemaphore(semaphoreInfo);
    auto resFen = logicalDevice.createFence(fenceInfo);
    if (resImgAvl.result != vk::Result::eSuccess ||
        resRendFin.result != vk::Result::eSuccess ||
        resFen.result != vk::Result::eSuccess) {
      throw std::runtime_error("Failed to create synchronization objects");
    }

    imageAvailableSemaphores[i] = resImgAvl.value;
    renderFinishedSemaphores[i] = resRendFin.value;
    inFlightFences[i] = resFen.value;
  }

  deviceGlobalGarbageQueue.push_back([this](vk::Device device) {
    for (int i = 0; i < framesInFlight; i++) {
      device.destroySemaphore(imageAvailableSemaphores[i]);
      device.destroySemaphore(renderFinishedSemaphores[i]);
      device.destroyFence(inFlightFences[i]);
    }
  });
}

vk::Format
VulkanRender::findSupportedFormat(const std::vector<vk::Format> &candidates,
                                  vk::ImageTiling tiling,
                                  vk::FormatFeatureFlagBits features) {
  for (vk::Format format : candidates) {
    vk::FormatProperties props;
    physicalDevice.getFormatProperties(format, &props);

    if (tiling == vk::ImageTiling::eLinear &&
        (props.linearTilingFeatures & features) == features) {
      return format;
    } else if (tiling == vk::ImageTiling::eOptimal &&
               (props.optimalTilingFeatures & features) == features) {
      return format;
    }
  }

  throw std::runtime_error("failed to find supported format!");
}

vk::Format VulkanRender::findDepthFormat() {
  return findSupportedFormat(
      {vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint,
       vk::Format::eD24UnormS8Uint},
      vk::ImageTiling::eOptimal,
      vk::FormatFeatureFlagBits::eDepthStencilAttachment);
}

bool VulkanRender::hasStencilComponent(vk::Format format) {
  return format == vk::Format::eD32SfloatS8Uint ||
         format == vk::Format::eD24UnormS8Uint;
}

void VulkanRender::initIMGUI() {
  // Create descriptor pool for ImGui
  std::array<vk::DescriptorPoolSize, 11> poolSizes = {
      vk::DescriptorPoolSize(vk::DescriptorType::eSampler, 1000),
      vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, 1000),
      vk::DescriptorPoolSize(vk::DescriptorType::eSampledImage, 1000),
      vk::DescriptorPoolSize(vk::DescriptorType::eStorageImage, 1000),
      vk::DescriptorPoolSize(vk::DescriptorType::eUniformTexelBuffer, 1000),
      vk::DescriptorPoolSize(vk::DescriptorType::eStorageTexelBuffer, 1000),
      vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, 1000),
      vk::DescriptorPoolSize(vk::DescriptorType::eStorageBuffer, 1000),
      vk::DescriptorPoolSize(vk::DescriptorType::eUniformBufferDynamic, 1000),
      vk::DescriptorPoolSize(vk::DescriptorType::eStorageBufferDynamic, 1000),
      vk::DescriptorPoolSize(vk::DescriptorType::eInputAttachment, 1000)};

  vk::DescriptorPoolCreateInfo poolInfo(
      vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
      1000, // maxSets
      static_cast<uint32_t>(poolSizes.size()), poolSizes.data());

  auto resDP = logicalDevice.createDescriptorPool(poolInfo);
  if (resDP.result != vk::Result::eSuccess) {
    throw std::runtime_error("Failed to create imgui descriptor pool");
  }
  imguiDescriptorPool = resDP.value;

  // Initialize ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  io.IniFilename = nullptr;

  // Initialize GLFW backend
  ImGui_ImplGlfw_InitForVulkan(window, true);

  // Initialize Vulkan backend
  ImGui_ImplVulkan_InitInfo init_info = {};
  init_info.Instance = static_cast<VkInstance>(instance);
  init_info.PhysicalDevice = static_cast<VkPhysicalDevice>(physicalDevice);
  init_info.Device = static_cast<VkDevice>(logicalDevice);
  init_info.QueueFamily = findQueueFamilyIndex(vk::QueueFlagBits::eGraphics);
  init_info.Queue = static_cast<VkQueue>(graphicsQueue);
  init_info.PipelineCache = VK_NULL_HANDLE;
  init_info.DescriptorPool = static_cast<VkDescriptorPool>(imguiDescriptorPool);
  init_info.RenderPass = static_cast<VkRenderPass>(renderPass);
  init_info.Subpass = 0;
  init_info.MinImageCount = chain.imageCount;
  init_info.ImageCount = chain.imageCount;
  init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
  init_info.Allocator = nullptr;
  init_info.CheckVkResultFn = nullptr;

  if (!ImGui_ImplVulkan_Init(&init_info)) {
    throw std::runtime_error("Failed to initialize ImGui Vulkan backend");
  }

  ImGui_ImplVulkan_CreateFontsTexture();
}

void VulkanRender::waitIdle() { logicalDevice.waitIdle(); }

void VulkanRender::switchResized() { frameBufferResized = !frameBufferResized; }

void VulkanRender::setCurrentFps(int _fps) { currentFPS = _fps; }

void VulkanRender::showPerformanceMenu() {

  ImGui::Begin("FPS", nullptr,
               ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove |
                   ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBackground |
                   ImGuiWindowFlags_AlwaysAutoResize);
  ImGui::SetWindowPos(ImVec2(0, 0), ImGuiCond_Always);
  ImGui::Text("FPS: %d", currentFPS);
  ImGui::End();
}

void VulkanRender::updateState() {
  std::function<void()> command;
  {
    std::lock_guard<std::mutex> lock(*renderQueueMutex);
    if (!renderCommands->empty()) {
      command = renderCommands->front();
      renderCommands->pop();
    }
  }
  if (command)
    command();
}

void VulkanRender::showMenu() {}
