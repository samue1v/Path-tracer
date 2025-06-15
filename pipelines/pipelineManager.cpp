#include "pipelineManager.hpp"
#include <iostream>
PipelineManager::PipelineManager(vk::Device _logicalDevice,
                                 vk::RenderPass _renderPass)
    : logicalDevice(_logicalDevice), renderPass(_renderPass) {
 // createDescriptorSetLayout();
}

std::pair<vk::Pipeline,vk::PipelineLayout> PipelineManager::loadPipeline(PipelineInstance inst) {
  if (auto p = pipelineMap.find(inst); p != pipelineMap.end()) {
    return p->second;
  }
  switch (inst.type) {

  default:

    return {vk::Pipeline{},vk::PipelineLayout{}};
    break;
  }
}

void PipelineManager::createDescriptorSetLayout() {

  vk::DescriptorSetLayoutBinding layoutBinding{};
  layoutBinding.binding = 0;
  layoutBinding.descriptorType = vk::DescriptorType::eUniformBuffer;
  layoutBinding.descriptorCount = 1;
  layoutBinding.stageFlags = vk::ShaderStageFlagBits::eVertex;
  layoutBinding.pImmutableSamplers = nullptr;

  vk::DescriptorSetLayoutCreateInfo layoutInfo{};
  layoutInfo.sType = vk::StructureType::eDescriptorSetLayoutCreateInfo;
  layoutInfo.bindingCount = 1;
  layoutInfo.pBindings = &layoutBinding;

  auto layoutRes = logicalDevice.createDescriptorSetLayout(layoutInfo);
  if (layoutRes.result != vk::Result::eSuccess) {
    throw std::runtime_error("Failed to create descriptor set layout");
  }

  this->descriptorSetLayout = layoutRes.value;

  deletionQueue.push_back([this](vk::Device device) {
    device.destroyDescriptorSetLayout(this->descriptorSetLayout);
  });
}


void PipelineManager::createDescriptorPool(int framesInFlight) {
  vk::DescriptorPoolSize poolSize{};
  poolSize.type = vk::DescriptorType::eUniformBuffer;
  poolSize.descriptorCount = static_cast<uint32_t>(framesInFlight);

  vk::DescriptorPoolCreateInfo poolInfo{};
  poolInfo.sType = vk::StructureType::eDescriptorPoolCreateInfo;
  poolInfo.poolSizeCount = 1;
  poolInfo.pPoolSizes = &poolSize;
  poolInfo.maxSets = static_cast<uint32_t>(framesInFlight);

  auto res = logicalDevice.createDescriptorPool(poolInfo);
  if (res.result != vk::Result::eSuccess) {
    throw std::runtime_error("Failed to create descriptor pool");
  }

  descriptorPool = res.value;
  deletionQueue.push_back([this](vk::Device device){device.destroyDescriptorPool(descriptorPool,nullptr);});
}

vk::DescriptorSetLayout PipelineManager::getDescriptorSetLayout() const{
  return descriptorSetLayout;
}


vk::DescriptorPool PipelineManager::getDescriptorPool() const{
  return descriptorPool;
}

void PipelineManager::cleanUp() {

  while (deletionQueue.size() > 0) {

    deletionQueue.back()(logicalDevice);
    deletionQueue.pop_back();
  }
}
