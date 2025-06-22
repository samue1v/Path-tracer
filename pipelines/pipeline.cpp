
#include "pipeline.hpp"
#include <filesystem>
#include <fstream>
#include <unistd.h>

void Pipeline::createPipeline(
    vk::Device device, const char *shaderPath, const char *fragShaderPath,
    vk::RenderPass renderPass, vk::DescriptorSetLayout descriptorLayout,
    std::deque<std::function<void(vk::Device)>> &deletionQueue) {}

void Pipeline::createPipeline(
    vk::Device device, const char *shaderPath,
    vk::DescriptorSetLayout descriptorSetLayout,
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
                deletionQueue);
}

void Pipeline::buildPipeline(
    vk::Device device, const char *shaderPath,
    vk::DescriptorSetLayout descriptorSetLayout,
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
