#define VMA_IMPLEMENTATION
#include "vulkan_renderer.hpp"
#include "../config/config.hpp"
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "app.hpp"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_vulkan.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

VulkanRender::VulkanRender(GLFWwindow *window, const char *appName)
    : window(window), appName(appName), framesInFlight(1), currentFrame(0),
      frameBufferResized(0), firstFrame(true) {}

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

  // buildRenderPass();

  buildSwapchainImages();

  createImageResources();

  initializeScene();

  execScene(0);

  createUniformBuffers();

  createBufferResources();

  initializeBuffers();

  createSamplers();

  createDescriptorSetLayout();

  createPipeline();

  createDescriptorPool();

  createDescriptorSets();

  createCommandBuffers();

  initIMGUI();
}

VulkanRender::~VulkanRender() {

  ImGui_ImplVulkan_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
  logicalDevice.destroyDescriptorPool(imguiDescriptorPool);

  chain.cleanUp();

  compute.computeImg.cleanUp();

  // graphics.displayImg.cleanUp();

  compute.uniformBuffer.cleanUp();

  compute.hitDataBuffer.cleanUp();

  compute.pixelDataBuffer.cleanUp();

  compute.RNGbuffer.cleanUp();

  compute.spheresBuffer.cleanUp();

  compute.planesBuffer.cleanUp();

  vmaDestroyAllocator(this->allocator);

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

  VmaVulkanFunctions vkFunctions{};
  vkFunctions.vkGetInstanceProcAddr = &vkGetInstanceProcAddr;
  vkFunctions.vkGetDeviceProcAddr = &vkGetDeviceProcAddr;

  VmaAllocatorCreateInfo createInfo{};
  createInfo.physicalDevice = physicalDevice;
  createInfo.device = logicalDevice;
  createInfo.pHeapSizeLimit = nullptr;
  createInfo.instance = instance;
  createInfo.pTypeExternalMemoryHandleTypes = nullptr;
  createInfo.pVulkanFunctions = &vkFunctions;

  if (vmaCreateAllocator(&createInfo, &allocator) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create vma allocator");
  }
}

void VulkanRender::createUniformBuffers() {
  vk::BufferUsageFlags usageFlags = vk::BufferUsageFlagBits::eUniformBuffer |
                                    vk::BufferUsageFlagBits::eTransferDst;
  VkMemoryPropertyFlags allocFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
  vk::BufferCreateInfo bufferInfo{};
  bufferInfo.sType = vk::StructureType::eBufferCreateInfo;
  bufferInfo.usage = usageFlags;
  bufferInfo.size = sizeof(Tracer::camera);
  bufferInfo.sharingMode = vk::SharingMode::eExclusive;
  compute.uniformBuffer.create(
      logicalDevice, allocator, bufferInfo,
      VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO_PREFER_HOST, allocFlags);
}

void VulkanRender::createImageResources() {
  const vk::Format fmt = vk::Format::eR8G8B8A8Unorm;
  vk::FormatProperties fmtProps = physicalDevice.getFormatProperties(fmt);
  assert(fmtProps.optimalTilingFeatures &
         vk::FormatFeatureFlagBits::eStorageImage);

  std::array<uint32_t, 1> qFamilies({compute.familyQueueIDX});

  vk::ImageCreateInfo createInfo{};
  createInfo.sType = vk::StructureType::eImageCreateInfo;
  createInfo.pNext = nullptr;
  createInfo.imageType = vk::ImageType::e2D;
  createInfo.format = fmt;
  createInfo.extent = {{chain.extent.width, chain.extent.height, 1}};
  createInfo.mipLevels = 1;
  createInfo.arrayLayers = 1;
  createInfo.samples = vk::SampleCountFlagBits::e1;
  createInfo.tiling = vk::ImageTiling::eOptimal;
  createInfo.sharingMode = vk::SharingMode::eExclusive;
  createInfo.pQueueFamilyIndices = nullptr;
  createInfo.queueFamilyIndexCount = 0;
  createInfo.usage = vk::ImageUsageFlagBits::eStorage |
                     vk::ImageUsageFlagBits::eSampled |
                     vk::ImageUsageFlagBits::eTransferSrc |
                     vk::ImageUsageFlagBits::eTransferDst;
  createInfo.initialLayout = vk::ImageLayout::eUndefined;

  compute.computeImg.createImage(logicalDevice, allocator, createInfo);
  compute.computeImg.createView(vk::ImageAspectFlagBits::eColor);

  vk::CommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = vk::StructureType::eCommandBufferAllocateInfo;
  allocInfo.level = vk::CommandBufferLevel::ePrimary;
  allocInfo.commandPool = compute.commandPool;
  allocInfo.commandBufferCount = 1;

  vk::CommandBuffer commandBuffer;
  logicalDevice.allocateCommandBuffers(&allocInfo, &commandBuffer);

  vk::CommandBufferBeginInfo beginInfo{};
  beginInfo.sType = vk::StructureType::eCommandBufferBeginInfo;
  beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

  commandBuffer.begin(beginInfo);

  transitionImage(commandBuffer, compute.computeImg,
                  vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral,
                  vk::ImageAspectFlagBits::eColor);

  commandBuffer.end();

  vk::SubmitInfo submitInfo{};
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;

  compute.queue.submit(1, &submitInfo, nullptr);
  compute.queue.waitIdle();

  logicalDevice.freeCommandBuffers(compute.commandPool, 1, &commandBuffer);
}

void VulkanRender::createBufferResources() {
  vk::BufferUsageFlags usageFlags = vk::BufferUsageFlagBits::eStorageBuffer |
                                    vk::BufferUsageFlagBits::eTransferDst;
  VkMemoryPropertyFlags allocFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

  vk::BufferCreateInfo bufferInfoData{};
  bufferInfoData.sType = vk::StructureType::eBufferCreateInfo;
  bufferInfoData.usage = usageFlags;
  bufferInfoData.size = sizeof(Tracer::hitData) * compute.rays_per_pixel *
                        chain.extent.width * chain.extent.height;
  bufferInfoData.sharingMode = vk::SharingMode::eExclusive;
  compute.hitDataBuffer.create(
      logicalDevice, allocator, bufferInfoData,
      VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, allocFlags);

  vk::BufferCreateInfo bufferInfoRNG{};
  bufferInfoRNG.sType = vk::StructureType::eBufferCreateInfo;
  bufferInfoRNG.usage = usageFlags;
  bufferInfoRNG.size =
      sizeof(Tracer::PRNG32) * chain.extent.width * chain.extent.height;
  bufferInfoRNG.sharingMode = vk::SharingMode::eExclusive;
  compute.RNGbuffer.create(logicalDevice, allocator, bufferInfoRNG,
                           VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
                           allocFlags);

  vk::BufferCreateInfo bufferInfoSpheres{};
  bufferInfoSpheres.sType = vk::StructureType::eBufferCreateInfo;
  bufferInfoSpheres.usage = usageFlags;
  bufferInfoSpheres.size = sizeof(Tracer::sphere) * compute.MAX_OBJECT_SIZE;
  bufferInfoSpheres.sharingMode = vk::SharingMode::eExclusive;
  compute.spheresBuffer.create(
      logicalDevice, allocator, bufferInfoSpheres,
      VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, allocFlags);

  vk::BufferCreateInfo bufferInfoPlanes{};
  bufferInfoPlanes.sType = vk::StructureType::eBufferCreateInfo;
  bufferInfoPlanes.usage = usageFlags;
  bufferInfoPlanes.size = sizeof(Tracer::plane) * compute.MAX_OBJECT_SIZE;
  bufferInfoPlanes.sharingMode = vk::SharingMode::eExclusive;
  compute.planesBuffer.create(
      logicalDevice, allocator, bufferInfoPlanes,
      VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, allocFlags);

  vk::BufferCreateInfo bufferInfoPixel{};
  bufferInfoPixel.sType = vk::StructureType::eBufferCreateInfo;
  bufferInfoPixel.usage = usageFlags;
  bufferInfoPixel.size =
      sizeof(Tracer::pixelData) * chain.extent.width * chain.extent.height;
  bufferInfoPixel.sharingMode = vk::SharingMode::eExclusive;
  compute.pixelDataBuffer.create(
      logicalDevice, allocator, bufferInfoPixel,
      VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, allocFlags);
}

void VulkanRender::initializeBuffers() {

  // Initialize Uniform
  CameraPositionOperator camera_op(compute.cameraUniform);
  camera_op.doOperation(logicalDevice, compute.commandPool, compute.queue,
                        compute.uniformBuffer, 1, allocator);

  // Initialize SSBOs
  TauswortheOperator rd_op;
  MultiJitterOperator mj_op(chain.extent.width, chain.extent.height,
                            compute.rays_per_pixel, compute.cameraUniform);

  rd_op.doOperation(logicalDevice, compute.commandPool, compute.queue,
                    compute.RNGbuffer, chain.extent.width * chain.extent.height,
                    allocator);

  mj_op.doOperation(
      logicalDevice, compute.commandPool, compute.queue, compute.hitDataBuffer,
      chain.extent.width * chain.extent.height * compute.rays_per_pixel,
      allocator);

  PixelFillOperator pixel_op;
  pixel_op.doOperation(logicalDevice, compute.commandPool, compute.queue,
                       compute.pixelDataBuffer,
                       chain.extent.width * chain.extent.height, allocator);

  SphereFillOperator sphere_op(compute.spheres);
  PlaneFillOperator plane_op(compute.planes);

  sphere_op.doOperation(logicalDevice, compute.commandPool, compute.queue,
                        compute.spheresBuffer, compute.MAX_OBJECT_SIZE,
                        allocator);
  plane_op.doOperation(logicalDevice, compute.commandPool, compute.queue,
                       compute.planesBuffer, compute.MAX_OBJECT_SIZE,
                       allocator);
}

void VulkanRender::updateShaderData(vk::CommandBuffer cmdBuffer) {
  // Update constant;
  //
  compute.constants.numPlanes = compute.planes.size();
  compute.constants.numSpheres = compute.spheres.size();
  compute.constants.numLights = 0;
  compute.constants.rpp = compute.rays_per_pixel;
  compute.constants.camera_move = (uint32_t)*cameraMoved;
  compute.constants.max_rpp = compute.MAX_RAYS_PER_PIXEL;

  int ri = readIndex->load(std::memory_order_relaxed);
  compute.constants.m = (*buffer_camera).at(ri).invView;

  cmdBuffer.pushConstants(compute.pipeline.layout,
                          vk::ShaderStageFlagBits::eCompute, 0,
                          sizeof(Tracer::PushConstants), &compute.constants);
}

void VulkanRender::drawFrame() {

  logicalDevice.waitForFences(1, &inFlightFences[currentFrame], vk::True,
                              UINT64_MAX);

  auto resImg = logicalDevice.acquireNextImageKHR(
      chain.chain, UINT64_MAX, imageAvailableSemaphores[currentFrame], nullptr);

  if (resImg.result == vk::Result::eErrorOutOfDateKHR) {
    return;
  } else if (resImg.result != vk::Result::eSuccess &&
             resImg.result != vk::Result::eSuboptimalKHR) {
    throw std::runtime_error("failed to acquire swap chain image!");
  }

  uint32_t imageIdx = resImg.value;

  ImGui_ImplVulkan_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();
  showPerformanceMenu();
  // showMenu();

  if (*cameraMoved) {
    Logger::log(Logger::LogLevel::DEBUG, "Camera moved need reset");
    compute.constants.camera_move =
        (*cameraMoved).load(std::memory_order_acquire);
  }

  recordComputeCommandBuffer(compute.commandBuffer);

  vk::Semaphore computeWaitSemaphores[1];
  vk::PipelineStageFlags computeWaitStages[1];

  vk::SubmitInfo submitInfoCompute{};

  if (!firstFrame) {
    computeWaitSemaphores[0] = graphics.renderFinishedSemaphore;
    computeWaitStages[0] = vk::PipelineStageFlagBits::eComputeShader;
    submitInfoCompute.waitSemaphoreCount = 1;
    submitInfoCompute.pWaitSemaphores = computeWaitSemaphores;
    submitInfoCompute.pWaitDstStageMask = computeWaitStages;
  } else {
    submitInfoCompute.waitSemaphoreCount = 0;
    submitInfoCompute.pWaitSemaphores = nullptr;
    submitInfoCompute.pWaitDstStageMask = nullptr;
    firstFrame = false;
  }

  submitInfoCompute.sType = vk::StructureType::eSubmitInfo;
  submitInfoCompute.commandBufferCount = 1;
  submitInfoCompute.pCommandBuffers = &compute.commandBuffer;
  submitInfoCompute.signalSemaphoreCount = 1;
  submitInfoCompute.pSignalSemaphores = &compute.computeFinishedSemaphore;

  compute.queue.submit(1, &submitInfoCompute, nullptr);


  if (enableValidationLayers) {
    auto now = std::chrono::system_clock::now();

    std::time_t now_time_t = std::chrono::system_clock::to_time_t(now);

    std::tm local_tm = *std::localtime(&now_time_t);

    std::ostringstream oss;
    oss << std::put_time(&local_tm, "%Y-%m-%d %H:%M:%S");

    Logger::log(Logger::LogLevel::DEBUG, "Compute done at: " + oss.str());
  }

  // logicalDevice.waitForFences(1,&compute.computeFence,1, UINT64_MAX);
  //}

  logicalDevice.resetFences(1, &inFlightFences[currentFrame]);

  graphics.commandBuffer[currentFrame].reset();

  recordGraphicCommandBuffer(graphics.commandBuffer[currentFrame], imageIdx);

  vk::Semaphore graphicsWaitSemaphores[] = {
      imageAvailableSemaphores[currentFrame], compute.computeFinishedSemaphore};

  vk::Semaphore graphicsSignalSemaphores[] = {
      renderFinishedSemaphores[currentFrame], graphics.renderFinishedSemaphore};

  vk::PipelineStageFlags graphicsWaitStages[] = {
      vk::PipelineStageFlagBits::eColorAttachmentOutput,
      vk::PipelineStageFlagBits::eFragmentShader};

  vk::SubmitInfo submitInfo{};
  submitInfo.sType = vk::StructureType::eSubmitInfo;
  submitInfo.waitSemaphoreCount = 2;
  submitInfo.pWaitSemaphores = graphicsWaitSemaphores;
  submitInfo.pWaitDstStageMask = graphicsWaitStages;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &graphics.commandBuffer[currentFrame];
  submitInfo.signalSemaphoreCount = 2;
  submitInfo.pSignalSemaphores = graphicsSignalSemaphores;

  auto resSubmit =
      graphics.queue.submit(1, &submitInfo, inFlightFences[currentFrame]);
  assert(resSubmit == vk::Result::eSuccess);

  vk::PresentInfoKHR presentInfo{};
  presentInfo.sType = vk::StructureType::ePresentInfoKHR;
  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = &renderFinishedSemaphores[currentFrame];

  vk::SwapchainKHR swapChains[] = {chain.chain};

  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = swapChains;
  presentInfo.pImageIndices = &imageIdx;
  presentInfo.pResults = nullptr;

  auto resPres = graphics.queue.presentKHR(&presentInfo);
  (*cameraMoved).store(0, std::memory_order_relaxed);

  // if (resPres == vk::Result::eErrorOutOfDateKHR ||
  //     resPres == vk::Result::eSuboptimalKHR || frameBufferResized) {
  //   throw std::runtime_error("Window resized error.");
  // } else if (resPres != vk::Result::eSuccess &&
  //            resPres != vk::Result::eSuboptimalKHR) {
  //   throw std::runtime_error("Failed to present swap chain image.");
  // }
  currentFrame = (currentFrame + 1) % framesInFlight;
}

void VulkanRender::transitionImage(vk::CommandBuffer cmd, vk::Image image,
                                   vk::ImageLayout oldLayout,
                                   vk::ImageLayout newLayout,
                                   vk::ImageAspectFlags aspectMask) {

  vk::ImageMemoryBarrier barrier{};
  barrier.sType = vk::StructureType::eImageMemoryBarrier;
  barrier.oldLayout = oldLayout;
  barrier.newLayout = newLayout;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = image;

  barrier.subresourceRange.aspectMask = aspectMask;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;

  vk::PipelineStageFlags srcStage;
  vk::PipelineStageFlags dstStage;

  if (oldLayout == vk::ImageLayout::eUndefined &&
      newLayout == vk::ImageLayout::eGeneral) {

    barrier.srcAccessMask = {};
    barrier.dstAccessMask = vk::AccessFlagBits::eShaderWrite;

    srcStage = vk::PipelineStageFlagBits::eTopOfPipe;
    dstStage = vk::PipelineStageFlagBits::eComputeShader;
  }

  else if (oldLayout == vk::ImageLayout::ePresentSrcKHR &&
           newLayout == vk::ImageLayout::eColorAttachmentOptimal) {
    barrier.srcAccessMask = {};
    barrier.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;

    srcStage = vk::PipelineStageFlagBits::eTopOfPipe;
    dstStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;

  }

  else if (oldLayout == vk::ImageLayout::eGeneral &&
           newLayout == vk::ImageLayout::eColorAttachmentOptimal) {

    barrier.srcAccessMask =
        vk::AccessFlagBits::eShaderWrite | vk::AccessFlagBits::eShaderRead;
    barrier.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;

    srcStage = vk::PipelineStageFlagBits::eComputeShader;
    dstStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
  }

  else if (oldLayout == vk::ImageLayout::eColorAttachmentOptimal &&
           newLayout == vk::ImageLayout::ePresentSrcKHR) {

    barrier.srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
    barrier.dstAccessMask = {};

    srcStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    dstStage = vk::PipelineStageFlagBits::eBottomOfPipe;
  }

  else if (oldLayout == vk::ImageLayout::eGeneral &&
           newLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {

    barrier.srcAccessMask = vk::AccessFlagBits::eShaderWrite;
    barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

    srcStage = vk::PipelineStageFlagBits::eComputeShader;
    dstStage = vk::PipelineStageFlagBits::eFragmentShader;
  }

  else if (oldLayout == vk::ImageLayout::eShaderReadOnlyOptimal &&
           newLayout == vk::ImageLayout::eGeneral) {

    barrier.srcAccessMask = vk::AccessFlagBits::eShaderRead;
    barrier.dstAccessMask = vk::AccessFlagBits::eShaderWrite;

    srcStage = vk::PipelineStageFlagBits::eFragmentShader;
    dstStage = vk::PipelineStageFlagBits::eComputeShader;
  }

  else if (oldLayout == vk::ImageLayout::eUndefined &&
           newLayout == vk::ImageLayout::eColorAttachmentOptimal) {

    barrier.srcAccessMask = {};
    barrier.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;

    srcStage = vk::PipelineStageFlagBits::eTopOfPipe;
    dstStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
  }

  else {
    throw std::runtime_error("Unsupported layout transition!");
  }

  cmd.pipelineBarrier(srcStage, dstStage, vk::DependencyFlags{}, 0, nullptr, 0,
                      nullptr, 1, &barrier);
}

void VulkanRender::transitionImage(vk::CommandBuffer cmd, ResourceImage &image,
                                   vk::ImageLayout oldLayout,
                                   vk::ImageLayout newLayout,
                                   vk::ImageAspectFlags aspectMask) {

  transitionImage(cmd, image.image_, oldLayout, newLayout, aspectMask);
  image.currentLayout_ = newLayout;
}

void VulkanRender::transitionImage(vk::CommandBuffer cmd, Frame &frame,
                                   vk::ImageLayout oldLayout,
                                   vk::ImageLayout newLayout,
                                   vk::ImageAspectFlags aspectMask) {

  transitionImage(cmd, frame.image, oldLayout, newLayout, aspectMask);
  frame.layout = newLayout;
}

void VulkanRender::recordComputeCommandBuffer(vk::CommandBuffer cmdBuffer) {

  cmdBuffer.reset();

  vk::CommandBufferBeginInfo beginInfo{};
  beginInfo.sType = vk::StructureType::eCommandBufferBeginInfo;
  cmdBuffer.begin(&beginInfo);

  if (compute.computeImg.currentLayout_ == vk::ImageLayout::eUndefined) {
    transitionImage(cmdBuffer, compute.computeImg,
                    compute.computeImg.currentLayout_,
                    vk::ImageLayout::eGeneral, vk::ImageAspectFlagBits::eColor);
  } else if (compute.computeImg.currentLayout_ ==
             vk::ImageLayout::eShaderReadOnlyOptimal) {
    transitionImage(cmdBuffer, compute.computeImg,
                    compute.computeImg.currentLayout_,
                    vk::ImageLayout::eGeneral, vk::ImageAspectFlagBits::eColor);
  }

  updateShaderData(cmdBuffer);

  cmdBuffer.bindPipeline(vk::PipelineBindPoint::eCompute,
                         compute.pipeline.pipeline);
  cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute,
                               compute.pipeline.layout, 0, 1,
                               &general.descriptorSet, 0, nullptr);

  cmdBuffer.dispatch((chain.extent.width + 15) / 16,
                     (chain.extent.height + 15) / 16, 1);

  transitionImage(cmdBuffer, compute.computeImg, vk::ImageLayout::eGeneral,
                  vk::ImageLayout::eShaderReadOnlyOptimal,
                  vk::ImageAspectFlagBits::eColor);

  compute.commandBuffer.end();
}

void VulkanRender::createDescriptorSets() {

  vk::DescriptorSetAllocateInfo allocInfo;
  allocInfo.sType = vk::StructureType::eDescriptorSetAllocateInfo;
  allocInfo.descriptorPool = general.descriptorPool;
  allocInfo.descriptorSetCount = 1;
  allocInfo.pSetLayouts = &general.descriptorSetLayout;

  auto resAlloc = logicalDevice.allocateDescriptorSets(allocInfo);
  assert(resAlloc.result == vk::Result::eSuccess);
  general.descriptorSet = resAlloc.value[0];

  vk::DescriptorBufferInfo uniformBufferInfo{};
  uniformBufferInfo.buffer = compute.uniformBuffer.buffer;
  uniformBufferInfo.offset = 0;
  uniformBufferInfo.range = sizeof(Tracer::camera);

  vk::DescriptorBufferInfo storageBufferInfo{};
  storageBufferInfo.buffer = compute.hitDataBuffer.buffer;
  storageBufferInfo.offset = 0;
  storageBufferInfo.range = sizeof(Tracer::hitData) * compute.rays_per_pixel *
                            chain.extent.width * chain.extent.height;

  vk::DescriptorImageInfo imageInfoA{};
  imageInfoA.imageView = compute.computeImg.view_;
  imageInfoA.imageLayout = compute.computeImg.currentLayout_;

  vk::DescriptorBufferInfo rngBufferInfo{};
  rngBufferInfo.buffer = compute.RNGbuffer.buffer;
  rngBufferInfo.offset = 0;
  rngBufferInfo.range =
      sizeof(Tracer::PRNG32) * chain.extent.width * chain.extent.height;

  vk::DescriptorBufferInfo spheresBufferInfo{};
  spheresBufferInfo.buffer = compute.spheresBuffer.buffer;
  spheresBufferInfo.offset = 0;
  spheresBufferInfo.range = sizeof(Tracer::sphere) * compute.MAX_OBJECT_SIZE;

  vk::DescriptorBufferInfo planesBufferInfo{};
  planesBufferInfo.buffer = compute.planesBuffer.buffer;
  planesBufferInfo.offset = 0;
  planesBufferInfo.range = sizeof(Tracer::plane) * compute.MAX_OBJECT_SIZE;

  vk::DescriptorBufferInfo pixelBufferInfo{};
  pixelBufferInfo.buffer = compute.pixelDataBuffer.buffer;
  pixelBufferInfo.offset = 0;
  pixelBufferInfo.range =
      sizeof(Tracer::pixelData) * chain.extent.width * chain.extent.height;

  vk::DescriptorImageInfo samplerImageInfo{};
  samplerImageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
  samplerImageInfo.imageView = compute.computeImg.view_;
  samplerImageInfo.sampler = graphics.sampler;

  std::array<vk::WriteDescriptorSet, 8> descriptorWrites;

  descriptorWrites[0].sType = vk::StructureType::eWriteDescriptorSet;
  descriptorWrites[0].dstSet = general.descriptorSet;
  descriptorWrites[0].dstBinding = 0;
  descriptorWrites[0].dstArrayElement = 0;
  descriptorWrites[0].descriptorType = vk::DescriptorType::eUniformBuffer;
  descriptorWrites[0].descriptorCount = 1;
  descriptorWrites[0].pBufferInfo = &uniformBufferInfo;

  descriptorWrites[1].sType = vk::StructureType::eWriteDescriptorSet;
  descriptorWrites[1].dstSet = general.descriptorSet;
  descriptorWrites[1].dstBinding = 1;
  descriptorWrites[1].dstArrayElement = 0;
  descriptorWrites[1].descriptorType = vk::DescriptorType::eStorageBuffer;
  descriptorWrites[1].descriptorCount = 1;
  descriptorWrites[1].pBufferInfo = &storageBufferInfo;

  descriptorWrites[2].sType = vk::StructureType::eWriteDescriptorSet;
  descriptorWrites[2].dstSet = general.descriptorSet;
  descriptorWrites[2].dstBinding = 2;
  descriptorWrites[2].dstArrayElement = 0;
  descriptorWrites[2].descriptorType = vk::DescriptorType::eStorageImage;
  descriptorWrites[2].descriptorCount = 1;
  descriptorWrites[2].pImageInfo = &imageInfoA;

  descriptorWrites[3].sType = vk::StructureType::eWriteDescriptorSet;
  descriptorWrites[3].dstSet = general.descriptorSet;
  descriptorWrites[3].dstBinding = 3;
  descriptorWrites[3].dstArrayElement = 0;
  descriptorWrites[3].descriptorType = vk::DescriptorType::eStorageBuffer;
  descriptorWrites[3].descriptorCount = 1;
  descriptorWrites[3].pBufferInfo = &rngBufferInfo;

  descriptorWrites[4].sType = vk::StructureType::eWriteDescriptorSet;
  descriptorWrites[4].dstSet = general.descriptorSet;
  descriptorWrites[4].dstBinding = 4;
  descriptorWrites[4].dstArrayElement = 0;
  descriptorWrites[4].descriptorType = vk::DescriptorType::eStorageBuffer;
  descriptorWrites[4].descriptorCount = 1;
  descriptorWrites[4].pBufferInfo = &spheresBufferInfo;

  descriptorWrites[5].sType = vk::StructureType::eWriteDescriptorSet;
  descriptorWrites[5].dstSet = general.descriptorSet;
  descriptorWrites[5].dstBinding = 5;
  descriptorWrites[5].dstArrayElement = 0;
  descriptorWrites[5].descriptorType = vk::DescriptorType::eStorageBuffer;
  descriptorWrites[5].descriptorCount = 1;
  descriptorWrites[5].pBufferInfo = &planesBufferInfo;

  descriptorWrites[6].sType = vk::StructureType::eWriteDescriptorSet;
  descriptorWrites[6].dstSet = general.descriptorSet;
  descriptorWrites[6].dstBinding = 6;
  descriptorWrites[6].dstArrayElement = 0;
  descriptorWrites[6].descriptorType = vk::DescriptorType::eStorageBuffer;
  descriptorWrites[6].descriptorCount = 1;
  descriptorWrites[6].pBufferInfo = &pixelBufferInfo;

  descriptorWrites[7].sType = vk::StructureType::eWriteDescriptorSet;
  descriptorWrites[7].dstSet = general.descriptorSet;
  descriptorWrites[7].dstBinding = 7;
  descriptorWrites[7].dstArrayElement = 0;
  descriptorWrites[7].descriptorType =
      vk::DescriptorType::eCombinedImageSampler;
  descriptorWrites[7].descriptorCount = 1;
  descriptorWrites[7].pImageInfo = &samplerImageInfo;
  descriptorWrites[7].pBufferInfo = nullptr;
  descriptorWrites[7].pTexelBufferView = nullptr;

  logicalDevice.updateDescriptorSets(descriptorWrites, nullptr);
}

void VulkanRender::createInstance() {

  // Extensions
  instanceSupportedExtensions =
      vk::enumerateInstanceExtensionProperties().value;

  vk::ApplicationInfo appInfo =
      vk::ApplicationInfo(this->appName, vk::enumerateInstanceVersion().value,
                          this->appName, ENGINE_VERSION, vk::ApiVersion14);
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
        Logger::log(Logger::LogLevel::DEBUG,
                    "LAYER: " + std::string(supportedExtension.extensionName));
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
    vkGetPhysicalDeviceProperties(device, &prop);
    if (enableValidationLayers) {
      Logger::log(Logger::LogLevel::DEBUG,
                  "DEVICE: " + std::string(prop.deviceName));
    }
    if (isSuitable(device) &&
        prop.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
      physicalDevice = device;
      auto apiVersion = prop.apiVersion;
      uint32_t variant = VK_API_VERSION_VARIANT(apiVersion);
      uint32_t major = VK_API_VERSION_MAJOR(apiVersion);
      uint32_t minor = VK_API_VERSION_MINOR(apiVersion);
      uint32_t patch = VK_API_VERSION_PATCH(apiVersion);

      Logger::log(Logger::LogLevel::INFO,
                  "USING DEVICE: " + std::string(prop.deviceName));
      Logger::log(Logger::LogLevel::INFO, "API: " + std::to_string(major) +
                                              "." + std::to_string(minor) +
                                              "." + std::to_string(patch) +
                                              "." + std::to_string(variant));

      VkPhysicalDeviceDriverProperties driverProps = {};
      driverProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRIVER_PROPERTIES;

      VkPhysicalDeviceProperties2 props2 = {};
      props2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
      props2.pNext = &driverProps;

      vkGetPhysicalDeviceProperties2(physicalDevice, &props2);

      Logger::log(Logger::LogLevel::INFO,
                  "Driver: " + std::string(driverProps.driverInfo));

      uint32_t maxInvocations = prop.limits.maxComputeWorkGroupInvocations;
      VkExtent3D maxSize = {prop.limits.maxComputeWorkGroupSize[0],
                            prop.limits.maxComputeWorkGroupSize[1],
                            prop.limits.maxComputeWorkGroupSize[2]};
      Logger::log(Logger::LogLevel::DEBUG,
                  "Max invocations: " + std::to_string(maxInvocations));
      Logger::log(Logger::LogLevel::DEBUG,
                  "X: " + std::to_string(maxSize.width) + " | " +
                      "Y: " + std::to_string(maxSize.height) + " | " +
                      "Z: " + std::to_string(maxSize.depth));

      Logger::log(
          Logger::LogLevel::DEBUG,
          "minUniformBufferOffsetAlignment: " +
              std::to_string(prop.limits.minUniformBufferOffsetAlignment));

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
          Logger::log(Logger::LogLevel::DEBUG, "EXT: " + name);
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

    // bool canPresent = true;
    // if (surface) {
    //   if (physicalDevice.getSurfaceSupportKHR(i, surface).result !=
    //       vk::Result::eSuccess) {
    //     canPresent = false;
    //   }
    // }

    bool canPresent = true;
    if (surface) {
      auto support = physicalDevice.getSurfaceSupportKHR(i, surface);
      if (support.result != vk::Result::eSuccess || support.value == false) {
        canPresent = false;
      }
    }

    bool supported = false;
    if (queueFamily.queueFlags & queueType) {
      supported = true;
    }

    // Compute only Queue
    if (queueType == vk::QueueFlagBits::eCompute) {
      if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics) {
        continue;
      }
      if (supported) {
        return i;
      }
    }

    // Graphics and present queue
    if (supported && canPresent) {
      return i;
    }
  }

  return UINT32_MAX;
}

void VulkanRender::createLogicalDevice() {

  uint32_t computeIndex = findQueueFamilyIndex(vk::QueueFlagBits::eGraphics);
  compute.familyQueueIDX = computeIndex;
  uint32_t graphicsIndex = findQueueFamilyIndex(vk::QueueFlagBits::eGraphics);
  graphics.familyQueueIDX = graphicsIndex;
  uint32_t presentIndex = graphicsIndex;

  float queuePriority[] = {1.f, 1.f};

  /*
  * VULKAN_HPP_CONSTEXPR DeviceQueueCreateInfo(
          VULKAN_HPP_NAMESPACE::DeviceQueueCreateFlags flags_	= {},
  uint32_t                          queueFamilyIndex_ = {},
  uint32_t                          queueCount_       = {},
  const float * pQueuePriorities_ = {} ) VULKAN_HPP_NOEXCEPT
  */
  vk::DeviceQueueCreateInfo queueCreateInfoGraphic = vk::DeviceQueueCreateInfo(
      vk::DeviceQueueCreateFlags(), graphicsIndex, 2, queuePriority);

  // vk::DeviceQueueCreateInfo queueCreateInfoCompute =
  // vk::DeviceQueueCreateInfo(
  //     vk::DeviceQueueCreateFlags(), computeIndex, 1, &queuePriority);

  VkDeviceQueueCreateInfo queueCreateGraphicHandle = queueCreateInfoGraphic;
  // VkDeviceQueueCreateInfo queueCreateComputeHandle = queueCreateInfoCompute;

  std::array<VkDeviceQueueCreateInfo, 1> queueInfos(
      {queueCreateGraphicHandle}); //, queueCreateComputeHandle});

  /*
   * Device features must be requested before the device is abstracted,
   * so that we only pay for what we need.
   */
  vk::PhysicalDeviceFeatures deviceFeatures = vk::PhysicalDeviceFeatures();

  VkPhysicalDeviceFeatures physicalDeviceFeaturesHandle = deviceFeatures;
  physicalDeviceFeaturesHandle.fillModeNonSolid = VK_TRUE;
  physicalDeviceFeaturesHandle.wideLines = VK_TRUE;

  vk::PhysicalDeviceDynamicRenderingFeaturesKHR dynamicFeatures;
  dynamicFeatures.sType =
      vk::StructureType::ePhysicalDeviceDynamicRenderingFeaturesKHR;
  dynamicFeatures.pNext = nullptr;
  dynamicFeatures.dynamicRendering = vk::True;

  std::vector<const char *> deviceExtensions;
  deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
  deviceExtensions.push_back(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);

  VkDeviceCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  createInfo.pNext = &dynamicFeatures;
  createInfo.queueCreateInfoCount = queueInfos.size();
  createInfo.pQueueCreateInfos = queueInfos.data();
  createInfo.enabledExtensionCount = deviceExtensions.size();
  createInfo.ppEnabledExtensionNames = deviceExtensions.data();
  createInfo.pEnabledFeatures = &physicalDeviceFeaturesHandle;

  VkDevice deviceHandle;

  if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &deviceHandle) !=
      VK_SUCCESS) {
    throw std::runtime_error("Could not create logical device");
  }

  logicalDevice = vk::Device(deviceHandle);

  graphics.queue = logicalDevice.getQueue(graphicsIndex, 0);
  graphics.queueIDX = 0;
  compute.queue = logicalDevice.getQueue(computeIndex, 1);
  compute.queueIDX = 1;

  Logger::log(Logger::LogLevel::DEBUG,
              {"Graphics Queue Family index: " + std::to_string(graphicsIndex),
               "Compute Queue Family index: " + std::to_string(computeIndex),
               "Compute Queue chosen: " + std::to_string(compute.queueIDX),
               "Graphics Queue chosen: " + std::to_string(graphics.queueIDX)});

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
  chain.create(logicalDevice, physicalDevice, surface, width, height,
               {graphics.queueIDX});
}

void VulkanRender::buildSwapchainImages() {
  chain.build();

  // vk::CommandBufferAllocateInfo allocInfo{};
  // allocInfo.sType = vk::StructureType::eCommandBufferAllocateInfo;
  // allocInfo.level = vk::CommandBufferLevel::ePrimary;
  // allocInfo.commandPool = graphics.commandPool;
  // allocInfo.commandBufferCount = 1;

  // vk::CommandBuffer commandBuffer;
  // logicalDevice.allocateCommandBuffers(&allocInfo, &commandBuffer);

  // vk::CommandBufferBeginInfo beginInfo{};
  // beginInfo.sType = vk::StructureType::eCommandBufferBeginInfo;
  // beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

  // commandBuffer.begin(&beginInfo);

  // for (Frame &f : chain.frames) {
  //   vk::ImageMemoryBarrier barrier{};
  //   barrier.oldLayout = vk::ImageLayout::eUndefined;
  //   barrier.newLayout = vk::ImageLayout::ePresentSrcKHR;
  //   barrier.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
  //   barrier.dstQueueFamilyIndex = vk::QueueFamilyIgnored;
  //   barrier.srcAccessMask = vk::AccessFlagBits::eNone;
  //   barrier.dstAccessMask = vk::AccessFlagBits::eNone;
  //   barrier.image = f.image;
  //   barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
  //   barrier.subresourceRange.baseMipLevel = 0;
  //   barrier.subresourceRange.levelCount = 1;
  //   barrier.subresourceRange.baseArrayLayer = 0;
  //   barrier.subresourceRange.layerCount = 1;

  //  commandBuffer.pipelineBarrier(
  //      vk::PipelineStageFlagBits::eTopOfPipe, // Adjust as needed
  //      vk::PipelineStageFlagBits::eBottomOfPipe, vk::DependencyFlags(),
  //      nullptr, nullptr, barrier);
  //}

  // commandBuffer.end();

  // vk::SubmitInfo submitInfo{};
  // submitInfo.sType = vk::StructureType::eSubmitInfo;
  // submitInfo.commandBufferCount = 1;
  // submitInfo.pCommandBuffers = &commandBuffer;

  // graphics.queue.submit(1, &submitInfo, vk::Fence{});
  // graphics.queue.waitIdle();
  // logicalDevice.freeCommandBuffers(graphics.commandPool, commandBuffer);
}

// void VulkanRender::buildRenderPass() {
//   vk::AttachmentDescription colorAttachment{};
//   colorAttachment.format = chain.format.format;
//   colorAttachment.samples = vk::SampleCountFlagBits::e1;
//   colorAttachment.loadOp = vk::AttachmentLoadOp::eLoad;
//   colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
//   colorAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
//   colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
//   colorAttachment.initialLayout = vk::ImageLayout::ePresentSrcKHR;
//   colorAttachment.finalLayout = vk::ImageLayout::ePresentSrcKHR;
//
//   vk::AttachmentReference colorAttachmentRef{};
//   colorAttachmentRef.attachment = 0;
//   colorAttachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal;
//
//   vk::SubpassDescription subpass{};
//   subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
//   subpass.colorAttachmentCount = 1;
//   subpass.pColorAttachments = &colorAttachmentRef;
//
//   vk::SubpassDependency dependency{};
//   dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
//   dependency.dstSubpass = 0;
//   dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput
//   |
//                             vk::PipelineStageFlagBits::eEarlyFragmentTests;
//
//   dependency.srcAccessMask = vk::AccessFlagBits::eNone;
//
//   dependency.dstStageMask =
//   vk::PipelineStageFlagBits::eColorAttachmentOutput; dependency.dstAccessMask
//   = vk::AccessFlagBits::eColorAttachmentWrite;
//   std::array<vk::AttachmentDescription, 1> attachments = {colorAttachment};
//
//   vk::RenderPassCreateInfo renderPassInfo{};
//   renderPassInfo.sType = vk::StructureType::eRenderPassCreateInfo;
//   renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
//   renderPassInfo.pAttachments = attachments.data();
//   renderPassInfo.subpassCount = 1;
//   renderPassInfo.pSubpasses = &subpass;
//   renderPassInfo.dependencyCount = 1;
//   renderPassInfo.pDependencies = &dependency;
//
//   auto res = logicalDevice.createRenderPass(renderPassInfo);
//   assert(res.result == vk::Result::eSuccess);
//
//   renderPass = res.value;
//   deviceGlobalGarbageQueue.push_back(
//       [this](vk::Device device) { device.destroyRenderPass(renderPass); });
// }

void VulkanRender::createCommandPool() {
  uint32_t graphicsFamilyIdx =
      graphics.queueIDX; // findQueueFamilyIndex(vk::QueueFlagBits::eGraphics);

  vk::CommandPoolCreateInfo poolInfo{};
  poolInfo.sType = vk::StructureType::eCommandPoolCreateInfo;
  poolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
  poolInfo.queueFamilyIndex = graphicsFamilyIdx;

  auto res = logicalDevice.createCommandPool(poolInfo);

  if (res.result != vk::Result::eSuccess) {
    throw std::runtime_error("failed to create graphics command pool!");
  }
  graphics.commandPool = res.value;
  deviceGlobalGarbageQueue.push_back([this](vk::Device device) {
    device.destroyCommandPool(graphics.commandPool);
  });

  uint32_t computeFamilyIdx =
      compute.queueIDX; // findQueueFamilyIndex(vk::QueueFlagBits::eCompute);

  poolInfo.queueFamilyIndex = graphicsFamilyIdx; // computeFamilyIdx;

  auto resCompute = logicalDevice.createCommandPool(poolInfo);

  if (resCompute.result != vk::Result::eSuccess) {
    throw std::runtime_error("failed to create graphics command pool!");
  }
  compute.commandPool = resCompute.value;
  deviceGlobalGarbageQueue.push_back([this](vk::Device device) {
    device.destroyCommandPool(compute.commandPool);
  });
}

void VulkanRender::createCommandBuffers() {
  graphics.commandBuffer.resize(framesInFlight);
  vk::CommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = vk::StructureType::eCommandBufferAllocateInfo;
  allocInfo.commandPool = graphics.commandPool;
  allocInfo.level = vk::CommandBufferLevel::ePrimary;
  allocInfo.commandBufferCount =
      static_cast<uint32_t>(graphics.commandBuffer.size());

  auto resGraph = logicalDevice.allocateCommandBuffers(
      &allocInfo, graphics.commandBuffer.data());
  assert(resGraph == vk::Result::eSuccess);

  vk::CommandBufferAllocateInfo computeAllocInfo{};
  computeAllocInfo.sType = vk::StructureType::eCommandBufferAllocateInfo;
  computeAllocInfo.commandPool = compute.commandPool;
  computeAllocInfo.level = vk::CommandBufferLevel::ePrimary;
  computeAllocInfo.commandBufferCount = 1;

  auto resComp = logicalDevice.allocateCommandBuffers(&computeAllocInfo,
                                                      &compute.commandBuffer);
  assert(resComp == vk::Result::eSuccess);

  // vk::CommandBufferAllocateInfo compBufferAllocInfo{};
  // compBufferAllocInfo.sType = vk::StructureType::eCommandBufferAllocateInfo;
  // compBufferAllocInfo.commandBufferCount = 1;
  // compBufferAllocInfo.commandPool = compute.commandPool;
  // compBufferAllocInfo.level = vk::CommandBufferLevel::ePrimary;

  // std::array<vk::CommandBuffer, 1> computeTransitionCmdBuffers;

  // auto resCompTrans = logicalDevice.allocateCommandBuffers(
  //     &compBufferAllocInfo, computeTransitionCmdBuffers.data());
  // assert(resCompTrans == vk::Result::eSuccess);

  // compute.transitionBuffer = computeTransitionCmdBuffers[0];

  // vk::CommandBufferAllocateInfo graphBufferAllocInfo{};
  // graphBufferAllocInfo.sType = vk::StructureType::eCommandBufferAllocateInfo;
  // graphBufferAllocInfo.commandBufferCount = 3;
  // graphBufferAllocInfo.commandPool = graphics.commandPool;
  // graphBufferAllocInfo.level = vk::CommandBufferLevel::ePrimary;

  // std::array<vk::CommandBuffer, 3> graphicTransitionCmdBuffers;

  // auto resGraphTrans = logicalDevice.allocateCommandBuffers(
  //     &graphBufferAllocInfo, graphicTransitionCmdBuffers.data());
  // assert(resGraphTrans == vk::Result::eSuccess);

  // graphics.acquireBuffer = graphicTransitionCmdBuffers[0];
  // graphics.releaseBuffer = graphicTransitionCmdBuffers[1];
  // graphics.copyBuffer = graphicTransitionCmdBuffers[2];
}

void VulkanRender::fromSwapchainToTransfer(vk::CommandBuffer commandBuffer,
                                           vk::Image img) {
  vk::ImageMemoryBarrier barrierSwapTransfer{};
  barrierSwapTransfer.image = img;
  barrierSwapTransfer.oldLayout = vk::ImageLayout::ePresentSrcKHR;
  barrierSwapTransfer.newLayout = vk::ImageLayout::eTransferDstOptimal;
  barrierSwapTransfer.srcQueueFamilyIndex = graphics.queueIDX;
  barrierSwapTransfer.dstQueueFamilyIndex = graphics.queueIDX;
  barrierSwapTransfer.subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1,
                                          0, 1};
  barrierSwapTransfer.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
  barrierSwapTransfer.srcAccessMask = vk::AccessFlagBits::eNone;

  commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe,
                                vk::PipelineStageFlagBits::eTransfer, {}, 0,
                                nullptr, 0, nullptr, 1, &barrierSwapTransfer);

  // compute.storageImg[1].transitionLayout(
  //     commandBuffer, {graphics.queueIDX, graphics.queueIDX},
  //     vk::ImageLayout::eGeneral, vk::ImageLayout::eTransferSrcOptimal,
  //     vk::AccessFlagBits::eShaderWrite, vk::AccessFlagBits::eNone,
  //     vk::PipelineStageFlagBits::eComputeShader,
  //     vk::PipelineStageFlagBits::eBottomOfPipe);
}

void VulkanRender::fromTransferToSwapchain(vk::CommandBuffer commandBuffer,
                                           vk::Image img) {
  vk::ImageMemoryBarrier barrierSwapPresent{};
  barrierSwapPresent.image = img;
  barrierSwapPresent.oldLayout = vk::ImageLayout::eTransferDstOptimal;
  barrierSwapPresent.newLayout = vk::ImageLayout::ePresentSrcKHR;
  barrierSwapPresent.srcQueueFamilyIndex = graphics.queueIDX;
  barrierSwapPresent.dstQueueFamilyIndex = graphics.queueIDX;
  barrierSwapPresent.subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1,
                                         0, 1};
  barrierSwapPresent.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
  barrierSwapPresent.dstAccessMask = vk::AccessFlagBits::eNone;

  commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                                vk::PipelineStageFlagBits::eBottomOfPipe, {}, 0,
                                nullptr, 0, nullptr, 1, &barrierSwapPresent);

  // compute.storageImg[compute.imageUsageType::display].transitionLayout(
  //     commandBuffer, {graphics.queueIDX, graphics.queueIDX},
  //     vk::ImageLayout::eTransferSrcOptimal, vk::ImageLayout::eGeneral,
  //     vk::AccessFlagBits::eNone, vk::AccessFlagBits::eShaderWrite,
  //     vk::PipelineStageFlagBits::eBottomOfPipe,
  //     vk::PipelineStageFlagBits::eComputeShader);
}

void VulkanRender::recordGraphicCommandBuffer(vk::CommandBuffer commandBuffer,
                                              uint32_t imageIndex) {

  vk::CommandBufferBeginInfo beginInfo{};
  beginInfo.sType = vk::StructureType::eCommandBufferBeginInfo;
  beginInfo.pInheritanceInfo = nullptr;

  auto resCmdBegin = commandBuffer.begin(&beginInfo);
  assert(resCmdBegin == vk::Result::eSuccess);

  if (chain.frames[imageIndex].layout == vk::ImageLayout::ePresentSrcKHR) {
    transitionImage(commandBuffer, chain.frames[imageIndex],
                    vk::ImageLayout::ePresentSrcKHR,
                    vk::ImageLayout::eColorAttachmentOptimal,
                    vk::ImageAspectFlagBits::eColor);
  }

  else if (chain.frames[imageIndex].layout == vk::ImageLayout::eUndefined) {
    transitionImage(commandBuffer, chain.frames[imageIndex],
                    vk::ImageLayout::eUndefined,
                    vk::ImageLayout::eColorAttachmentOptimal,
                    vk::ImageAspectFlagBits::eColor);
  }

  vk::RenderingAttachmentInfo colorAttachment{};
  colorAttachment.sType = vk::StructureType::eRenderingAttachmentInfo;
  colorAttachment.imageView = chain.frames[imageIndex].imageView;
  colorAttachment.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
  colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
  colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
  colorAttachment.clearValue.color =
      vk::ClearColorValue{57.f / 255.f, 62.f / 255.f, 70.f / 255.f, 1.f};

  vk::RenderingInfo renderingInfo{};
  renderingInfo.sType = vk::StructureType::eRenderingInfo;
  renderingInfo.renderArea = vk::Rect2D({0, 0}, chain.extent);
  renderingInfo.layerCount = 1;
  renderingInfo.colorAttachmentCount = 1;
  renderingInfo.pColorAttachments = &colorAttachment;

  commandBuffer.beginRendering(&renderingInfo);

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

  commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics,
                             graphics.pipeline.pipeline);

  commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                                   graphics.pipeline.layout, 0, 1,
                                   &general.descriptorSet, 0, nullptr);

  commandBuffer.draw(3, 1, 0, 0);

  ImGui::Render();
  ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(),
                                  static_cast<VkCommandBuffer>(commandBuffer));

  commandBuffer.endRendering();

  transitionImage(commandBuffer, chain.frames[imageIndex],
                  vk::ImageLayout::eColorAttachmentOptimal,
                  vk::ImageLayout::ePresentSrcKHR,
                  vk::ImageAspectFlagBits::eColor);

  commandBuffer.end();
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
    assert(resImgAvl.result == vk::Result::eSuccess &&
           resRendFin.result == vk::Result::eSuccess &&
           resFen.result == vk::Result::eSuccess);

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

  vk::SemaphoreCreateInfo semaphoreInfoCompute{};
  semaphoreInfoCompute.sType = vk::StructureType::eSemaphoreCreateInfo;

  auto semCompFin = logicalDevice.createSemaphore(semaphoreInfoCompute);
  auto semGraphRenderFin = logicalDevice.createSemaphore(semaphoreInfoCompute);
  assert(semCompFin.result == vk::Result::eSuccess &&
         semGraphRenderFin.result == vk::Result::eSuccess);
  compute.computeFinishedSemaphore = semCompFin.value;
  graphics.renderFinishedSemaphore = semGraphRenderFin.value;

  deviceGlobalGarbageQueue.push_back([this](vk::Device device) {
    // device.destroyFence(compute.computeFence);
    device.destroySemaphore(compute.computeFinishedSemaphore);
    device.destroySemaphore(graphics.renderFinishedSemaphore);
  });
}

void VulkanRender::createDescriptorSetLayout() {
  vk::DescriptorSetLayoutBinding uboLayoutBinding{};
  uboLayoutBinding.binding = 0;
  uboLayoutBinding.descriptorType = vk::DescriptorType::eUniformBuffer;
  uboLayoutBinding.descriptorCount = 1;
  uboLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eCompute;
  uboLayoutBinding.pImmutableSamplers = nullptr;

  vk::DescriptorSetLayoutBinding ssboLayoutBinding{};
  ssboLayoutBinding.binding = 1;
  ssboLayoutBinding.descriptorType = vk::DescriptorType::eStorageBuffer;
  ssboLayoutBinding.descriptorCount = 1;
  ssboLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eCompute;
  ssboLayoutBinding.pImmutableSamplers = nullptr;

  vk::DescriptorSetLayoutBinding imgLayoutBinding{};
  imgLayoutBinding.binding = 2;
  imgLayoutBinding.descriptorType = vk::DescriptorType::eStorageImage;
  imgLayoutBinding.descriptorCount = 1;
  imgLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eCompute;
  imgLayoutBinding.pImmutableSamplers = nullptr;

  vk::DescriptorSetLayoutBinding rngLayoutBinding{};
  rngLayoutBinding.binding = 3;
  rngLayoutBinding.descriptorType = vk::DescriptorType::eStorageBuffer;
  rngLayoutBinding.descriptorCount = 1;
  rngLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eCompute;
  rngLayoutBinding.pImmutableSamplers = nullptr;

  vk::DescriptorSetLayoutBinding spheresLayoutBinding{};
  spheresLayoutBinding.binding = 4;
  spheresLayoutBinding.descriptorType = vk::DescriptorType::eStorageBuffer;
  spheresLayoutBinding.descriptorCount = 1;
  spheresLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eCompute;
  spheresLayoutBinding.pImmutableSamplers = nullptr;

  vk::DescriptorSetLayoutBinding planesLayoutBinding{};
  planesLayoutBinding.binding = 5;
  planesLayoutBinding.descriptorType = vk::DescriptorType::eStorageBuffer;
  planesLayoutBinding.descriptorCount = 1;
  planesLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eCompute;
  planesLayoutBinding.pImmutableSamplers = nullptr;

  vk::DescriptorSetLayoutBinding pixelLayoutBinding{};
  pixelLayoutBinding.binding = 6;
  pixelLayoutBinding.descriptorType = vk::DescriptorType::eStorageBuffer;
  pixelLayoutBinding.descriptorCount = 1;
  pixelLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eCompute;
  pixelLayoutBinding.pImmutableSamplers = nullptr;

  vk::DescriptorSetLayoutBinding fragSamplerBinding{};
  fragSamplerBinding.binding = 7;
  fragSamplerBinding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
  fragSamplerBinding.descriptorCount = 1;
  fragSamplerBinding.stageFlags = vk::ShaderStageFlagBits::eFragment;
  fragSamplerBinding.pImmutableSamplers = nullptr;

  std::vector<vk::DescriptorSetLayoutBinding> bindings(
      {uboLayoutBinding, ssboLayoutBinding, imgLayoutBinding, rngLayoutBinding,
       spheresLayoutBinding, planesLayoutBinding, pixelLayoutBinding,
       fragSamplerBinding});

  vk::DescriptorSetLayoutCreateInfo createInfo{};
  createInfo.sType = vk::StructureType::eDescriptorSetLayoutCreateInfo;
  createInfo.bindingCount = bindings.size();
  createInfo.pBindings = bindings.data();

  auto res = logicalDevice.createDescriptorSetLayout(createInfo);
  assert(res.result == vk::Result::eSuccess);

  general.descriptorSetLayout = res.value;

  // Push constants
  compute.pushConstantsRange.offset = 0;
  compute.pushConstantsRange.size = sizeof(Tracer::PushConstants);
  compute.pushConstantsRange.stageFlags = vk::ShaderStageFlagBits::eCompute;

  deviceGlobalGarbageQueue.push_back(

      [this](vk::Device device) {
        device.destroyDescriptorSetLayout(general.descriptorSetLayout);
      });
}

void VulkanRender::createSamplers() {
  vk::SamplerCreateInfo samplerInfo{};
  samplerInfo.magFilter = vk::Filter::eNearest;
  samplerInfo.minFilter = vk::Filter::eNearest;
  samplerInfo.addressModeU = vk::SamplerAddressMode::eClampToEdge;
  samplerInfo.addressModeV = vk::SamplerAddressMode::eClampToEdge;
  samplerInfo.addressModeW = vk::SamplerAddressMode::eClampToEdge;

  auto res = logicalDevice.createSampler(samplerInfo);
  assert(res.result == vk::Result::eSuccess);
  graphics.sampler = res.value;

  deviceGlobalGarbageQueue.push_back([this](vk::Device device) {
    device.destroySampler(this->graphics.sampler);
  });
}

void VulkanRender::createPipeline() {

  compute.pipeline.createPipeline(
      logicalDevice, "pathTracer.comp.spv", general.descriptorSetLayout,
      compute.pushConstantsRange, deviceGlobalGarbageQueue);

  graphics.pipeline.createPipeline(
      logicalDevice, "vertTracer.vert.spv", "fragTracer.frag.spv",
      VK_NULL_HANDLE, general.descriptorSetLayout, compute.pushConstantsRange,
      deviceGlobalGarbageQueue);
}

void VulkanRender::createDescriptorPool() {
  std::array<vk::DescriptorPoolSize, 8> poolSizes = {};

  poolSizes[0].type = vk::DescriptorType::eUniformBuffer;
  poolSizes[0].descriptorCount = 1;

  poolSizes[1].type = vk::DescriptorType::eStorageBuffer;
  poolSizes[1].descriptorCount = 1;

  poolSizes[2].type = vk::DescriptorType::eStorageImage;
  poolSizes[2].descriptorCount = 1;

  poolSizes[3].type = vk::DescriptorType::eStorageBuffer;
  poolSizes[3].descriptorCount = 1;

  poolSizes[4].type = vk::DescriptorType::eStorageBuffer;
  poolSizes[4].descriptorCount = 1;

  poolSizes[5].type = vk::DescriptorType::eStorageBuffer;
  poolSizes[5].descriptorCount = 1;

  poolSizes[6].type = vk::DescriptorType::eStorageBuffer;
  poolSizes[6].descriptorCount = 1;

  poolSizes[7].type = vk::DescriptorType::eCombinedImageSampler;
  poolSizes[7].descriptorCount = 1;

  vk::DescriptorPoolCreateInfo poolInfo{};
  poolInfo.sType = vk::StructureType::eDescriptorPoolCreateInfo;
  poolInfo.poolSizeCount = poolSizes.size();
  poolInfo.pPoolSizes = poolSizes.data();
  poolInfo.maxSets = 1;

  auto res = logicalDevice.createDescriptorPool(poolInfo);
  assert(res.result == vk::Result::eSuccess);
  general.descriptorPool = res.value;

  deviceGlobalGarbageQueue.push_back([this](vk::Device device) {
    device.destroyDescriptorPool(this->general.descriptorPool);
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

bool VulkanRender::hasStencilComponent(vk::Format format) {
  return format == vk::Format::eD32SfloatS8Uint ||
         format == vk::Format::eD24UnormS8Uint;
}

void VulkanRender::initIMGUI() {
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
      vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet, 1000,
      static_cast<uint32_t>(poolSizes.size()), poolSizes.data());

  auto resDP = logicalDevice.createDescriptorPool(poolInfo);
  if (resDP.result != vk::Result::eSuccess) {
    throw std::runtime_error("Failed to create imgui descriptor pool");
  }
  imguiDescriptorPool = resDP.value;

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  io.IniFilename = nullptr;

  ImGui_ImplGlfw_InitForVulkan(window, true);

  std::vector<VkFormat> fmts;
  fmts.push_back(static_cast<VkFormat>(chain.frames[0].format));

  VkPipelineRenderingCreateInfoKHR pipelineCreateInfo;
  pipelineCreateInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
  pipelineCreateInfo.colorAttachmentCount = 1;
  pipelineCreateInfo.pColorAttachmentFormats = fmts.data();
  pipelineCreateInfo.pNext = nullptr;
  pipelineCreateInfo.viewMask = 0;
  pipelineCreateInfo.depthAttachmentFormat = VK_FORMAT_UNDEFINED;
  pipelineCreateInfo.stencilAttachmentFormat = VK_FORMAT_UNDEFINED;

  ImGui_ImplVulkan_InitInfo init_info = {};
  init_info.Instance = static_cast<VkInstance>(instance);
  init_info.PhysicalDevice = static_cast<VkPhysicalDevice>(physicalDevice);
  init_info.Device = static_cast<VkDevice>(logicalDevice);
  init_info.QueueFamily = findQueueFamilyIndex(vk::QueueFlagBits::eGraphics);
  init_info.Queue = static_cast<VkQueue>(graphics.queue);
  init_info.PipelineCache = VK_NULL_HANDLE;
  init_info.DescriptorPool = static_cast<VkDescriptorPool>(imguiDescriptorPool);
  // init_info.RenderPass = static_cast<VkRenderPass>(renderPass);
  init_info.UseDynamicRendering = true;
  init_info.PipelineRenderingCreateInfo = pipelineCreateInfo;
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

void VulkanRender::updateCamera(glm::vec3 posDelta, glm::vec3 atDelta) {}

void VulkanRender::showMenu() {
  static int rpp_menu;
  ImGui::SetNextWindowSize(ImVec2(0, 0), ImGuiCond_Always);
  ImGui::Begin("Menu", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
  ImGui::InputInt("RPP:", &rpp_menu);

  if (ImGui::Button("OK")) {
    compute.rays_per_pixel = rpp_menu * rpp_menu;
  }
  ImGui::End();
}
