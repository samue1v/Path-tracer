
#include "pipeline.hpp"
#include <fstream>




std::vector<char> Pipeline::readShader(const char *shaderFile) {
  std::ifstream file(shaderFile, std::ios::ate | std::ios::binary);

  if (!file.is_open()) {
    throw std::runtime_error("failed to open file!");
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
  createInfo.sType = vk::StructureType::
      eShaderModuleCreateInfo; // VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.codeSize = code.size();
  createInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());

  auto res = device.createShaderModule(createInfo);
  if (res.result != vk::Result::eSuccess) {
    throw std::runtime_error("failed to create shader module!");
  }

  return res.value;
}

