
#include "pipeline.hpp"
#include <filesystem>
#include <fstream>
#include <unistd.h>

void Pipeline::createPipeline(
    vk::Device device, const char *shaderPath, const char *fragShaderPath,
    vk::RenderPass renderPass, vk::DescriptorSetLayout descriptorLayout,
    vk::PushConstantRange pushConstantRange,
    std::deque<std::function<void(vk::Device)>> &deletionQueue) {

  char buffer[1024];
  ssize_t count = readlink("/proc/self/exe", buffer, sizeof(buffer));
  if (count == -1)
    throw std::runtime_error("Cannot read /proc/self/exe");

  auto exePath = std::filesystem::path(std::string(buffer, count));
  auto rootPath = exePath.parent_path().parent_path();
  auto shadersPath = rootPath / "shaders" / "compiled";
  std::string vertShaderSrc = shadersPath / shaderPath;
  std::string fragShaderSrc;
  if (fragShaderPath)
    fragShaderSrc = std::string(shadersPath / fragShaderPath);

  auto vertCode = readShader(vertShaderSrc.c_str());
  vk::ShaderModule vertModule = createShaderModule(vertCode, device);

  vk::ShaderModule fragModule = {};
  if (!fragShaderSrc.empty()) {
    auto fragCode = readShader(fragShaderSrc.c_str());
    fragModule = createShaderModule(fragCode, device);
  }

  vk::PipelineShaderStageCreateInfo vertStage{};
  vertStage.sType = vk::StructureType::ePipelineShaderStageCreateInfo;
  vertStage.stage = vk::ShaderStageFlagBits::eVertex;
  vertStage.module = vertModule;
  vertStage.pName = "main";

  std::vector<vk::PipelineShaderStageCreateInfo> stages;
  stages.push_back(vertStage);

  if (fragModule) {
    vk::PipelineShaderStageCreateInfo fragStage{};
    fragStage.sType = vk::StructureType::ePipelineShaderStageCreateInfo;
    fragStage.stage = vk::ShaderStageFlagBits::eFragment;
    fragStage.module = fragModule;
    fragStage.pName = "main";
    stages.push_back(fragStage);
  }

  vk::PipelineVertexInputStateCreateInfo vertexInputCI{};
  vertexInputCI.sType = vk::StructureType::ePipelineVertexInputStateCreateInfo;
  vertexInputCI.vertexBindingDescriptionCount = 0;
  vertexInputCI.vertexAttributeDescriptionCount = 0;
  vertexInputCI.pVertexBindingDescriptions = nullptr;
  vertexInputCI.pVertexAttributeDescriptions = nullptr;

  vk::PipelineInputAssemblyStateCreateInfo inputAssemblyCI{};
  inputAssemblyCI.sType =
      vk::StructureType::ePipelineInputAssemblyStateCreateInfo;
  inputAssemblyCI.topology = vk::PrimitiveTopology::eTriangleList;
  inputAssemblyCI.primitiveRestartEnable = VK_FALSE;

  // dynamic viewport & scissor
  std::array<vk::DynamicState, 2> dynamicStates = {vk::DynamicState::eViewport,
                                                   vk::DynamicState::eScissor};
  vk::PipelineDynamicStateCreateInfo dynamicStateCI{};
  dynamicStateCI.sType = vk::StructureType::ePipelineDynamicStateCreateInfo;
  dynamicStateCI.dynamicStateCount =
      static_cast<uint32_t>(dynamicStates.size());
  dynamicStateCI.pDynamicStates = dynamicStates.data();

  vk::PipelineRasterizationStateCreateInfo rasterCI{};
  rasterCI.sType = vk::StructureType::ePipelineRasterizationStateCreateInfo;
  rasterCI.depthClampEnable = VK_FALSE;
  rasterCI.rasterizerDiscardEnable = VK_FALSE;
  rasterCI.polygonMode = vk::PolygonMode::eFill;
  rasterCI.cullMode = vk::CullModeFlagBits::eNone;
  rasterCI.frontFace = vk::FrontFace::eClockwise;
  rasterCI.depthBiasEnable = VK_FALSE;
  rasterCI.lineWidth = 1.0f;

  vk::PipelineMultisampleStateCreateInfo multisampleCI{};
  multisampleCI.sType = vk::StructureType::ePipelineMultisampleStateCreateInfo;
  multisampleCI.rasterizationSamples = vk::SampleCountFlagBits::e1;
  multisampleCI.sampleShadingEnable = VK_FALSE;

  vk::PipelineDepthStencilStateCreateInfo depthStencilCI{};
  depthStencilCI.sType =
      vk::StructureType::ePipelineDepthStencilStateCreateInfo;

  vk::PipelineColorBlendAttachmentState colorBlendAttachment{};
  colorBlendAttachment.colorWriteMask =
      vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
      vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
  colorBlendAttachment.blendEnable = VK_FALSE;

  vk::PipelineColorBlendStateCreateInfo colorBlendCI{};
  colorBlendCI.sType = vk::StructureType::ePipelineColorBlendStateCreateInfo;
  colorBlendCI.logicOpEnable = VK_FALSE;
  colorBlendCI.attachmentCount = 1;
  colorBlendCI.pAttachments = &colorBlendAttachment;

  vk::PipelineViewportStateCreateInfo viewportCI{};
  viewportCI.sType = vk::StructureType::ePipelineViewportStateCreateInfo;
  viewportCI.viewportCount = 1;
  viewportCI.scissorCount = 1;

  vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
  pipelineLayoutInfo.sType = vk::StructureType::ePipelineLayoutCreateInfo;

  if (descriptorLayout != VK_NULL_HANDLE) {
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &descriptorLayout;
  } else {
    pipelineLayoutInfo.setLayoutCount = 0;
    pipelineLayoutInfo.pSetLayouts = nullptr;
  }

  if (pushConstantRange.size > 0) {
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
  } else {
    pipelineLayoutInfo.pushConstantRangeCount = 0;
    pipelineLayoutInfo.pPushConstantRanges = nullptr;
  }

  auto resLayout = device.createPipelineLayout(pipelineLayoutInfo);
  assert(resLayout.result == vk::Result::eSuccess);
  this->layout = resLayout.value;

  deletionQueue.push_back([layoutCopy = this->layout](vk::Device ldevice) {
    ldevice.destroyPipelineLayout(layoutCopy);
  });

  vk::GraphicsPipelineCreateInfo pipelineCI{};
  pipelineCI.sType = vk::StructureType::eGraphicsPipelineCreateInfo;
  pipelineCI.stageCount = static_cast<uint32_t>(stages.size());
  pipelineCI.pStages = stages.data();

  pipelineCI.pVertexInputState = &vertexInputCI;
  pipelineCI.pInputAssemblyState = &inputAssemblyCI;
  pipelineCI.pViewportState = &viewportCI;
  pipelineCI.pRasterizationState = &rasterCI;
  pipelineCI.pMultisampleState = &multisampleCI;
  pipelineCI.pDepthStencilState = &depthStencilCI;
  pipelineCI.pColorBlendState = &colorBlendCI;
  pipelineCI.pDynamicState = &dynamicStateCI;

  pipelineCI.layout = this->layout;

  vk::PipelineRenderingCreateInfo pipelineRenderingCI{};
  std::vector<vk::Format> colorFormats;
  if (renderPass != VK_NULL_HANDLE) {
    pipelineCI.renderPass = renderPass;
    pipelineCI.subpass = 0;
  } else {
    colorFormats.push_back(vk::Format::eB8G8R8A8Unorm);
    pipelineRenderingCI.sType = vk::StructureType::ePipelineRenderingCreateInfo;
    pipelineRenderingCI.colorAttachmentCount =
        static_cast<uint32_t>(colorFormats.size());
    pipelineRenderingCI.pColorAttachmentFormats = colorFormats.data();
    pipelineCI.pNext = &pipelineRenderingCI;
  }

  auto resPipe = device.createGraphicsPipeline(nullptr, pipelineCI);
  if (resPipe.result != vk::Result::eSuccess) {
    if (fragModule)
      device.destroyShaderModule(fragModule);
    device.destroyShaderModule(vertModule);
    throw std::runtime_error("failed to create graphics pipeline");
  }

  this->pipeline = resPipe.value;

  deletionQueue.push_back([pipelineCopy = this->pipeline](vk::Device ldevice) {
    ldevice.destroyPipeline(pipelineCopy);
  });

  if (fragModule)
    device.destroyShaderModule(fragModule);
  device.destroyShaderModule(vertModule);
}

void Pipeline::createPipeline(
    vk::Device device, const char *shaderPath,
    vk::DescriptorSetLayout descriptorSetLayout,
    vk::PushConstantRange pushConstantRange,
    std::deque<std::function<void(vk::Device)>> &deletionQueue) {

  char buffer[1024];
  ssize_t count = readlink("/proc/self/exe", buffer, sizeof(buffer));
  if (count == -1)
    throw std::runtime_error("Cannot read /proc/self/exe");

  auto exePath = std::filesystem::path(std::string(buffer, count));
  auto rootPath = exePath.parent_path().parent_path();
  auto shadersPath = rootPath / "shaders" / "compiled";
  std::string vertShaderSrc = shadersPath / shaderPath;

  buildPipeline(device, vertShaderSrc.c_str(), descriptorSetLayout,
                pushConstantRange, deletionQueue);
}

void Pipeline::buildPipeline(
    vk::Device device, const char *shaderPath,
    vk::DescriptorSetLayout descriptorSetLayout,
    vk::PushConstantRange pushConstantRange,
    std::deque<std::function<void(vk::Device)>> &deletionQueue) {
  auto shaderCode = readShader(shaderPath);

  vk::ShaderModule shaderModule = createShaderModule(shaderCode, device);

  vk::PipelineShaderStageCreateInfo stageInfo{};
  stageInfo.sType = vk::StructureType::ePipelineShaderStageCreateInfo;
  stageInfo.stage = vk::ShaderStageFlagBits::eCompute;
  stageInfo.module = shaderModule;
  stageInfo.pName = "main";

  vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
  pipelineLayoutInfo.sType = vk::StructureType::ePipelineLayoutCreateInfo;
  pipelineLayoutInfo.setLayoutCount = 1;
  pipelineLayoutInfo.pushConstantRangeCount = 0;
  pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
  pipelineLayoutInfo.pushConstantRangeCount = 1;
  pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

  auto resLayout = device.createPipelineLayout(pipelineLayoutInfo);

  assert(resLayout.result == vk::Result::eSuccess);
  this->layout = resLayout.value;
  deletionQueue.push_back([layoutCopy = layout](vk::Device ldevice) {
    ldevice.destroyPipelineLayout(layoutCopy);
  });

  vk::ComputePipelineCreateInfo pipelineInfo{};
  pipelineInfo.sType = vk::StructureType::eComputePipelineCreateInfo;
  pipelineInfo.stage = stageInfo;
  pipelineInfo.layout = layout;
  pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

  auto resPipe = device.createComputePipeline(nullptr, pipelineInfo);
  assert(resPipe.result == vk::Result::eSuccess);
  this->pipeline = resPipe.value;

  deletionQueue.push_back([pipelineCopy = pipeline](vk::Device ldevice) {
    ldevice.destroyPipeline(pipelineCopy);
  });

  device.destroyShaderModule(shaderModule);
}

std::vector<char> Pipeline::readShader(const char *shaderFile) {
  std::ifstream file(shaderFile, std::ios::ate | std::ios::binary);

  if (!file.is_open()) {
    throw std::runtime_error("failed to open shader file!");
  }

  size_t fileSize = (size_t)file.tellg();
  std::vector<char> buffer(fileSize);

  file.seekg(0);
  file.read(buffer.data(), fileSize);

  file.close();

  return buffer;
}

vk::ShaderModule Pipeline::createShaderModule(const std::vector<char> &code,
                                              vk::Device device) {

  vk::ShaderModuleCreateInfo createInfo{};
  createInfo.sType = vk::StructureType::eShaderModuleCreateInfo;
  createInfo.codeSize = code.size();
  createInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());

  auto res = device.createShaderModule(createInfo);
  if (res.result != vk::Result::eSuccess) {
    throw std::runtime_error("failed to create shader module!");
  }

  return res.value;
}
