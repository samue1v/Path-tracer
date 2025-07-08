#ifndef PIPELINE_HPP
#define PIPELINE_HPP
#define VULKAN_HPP_NO_EXCEPTIONS
#include "simpleMesh.hpp"
#include <deque>
#include <functional>
#include <vulkan/vulkan.hpp>

class Pipeline {
public:
  /**
   * @brief Default constructor
   */
  Pipeline() = default;

  /**
   * @brief Initializes graphics pipeline workflow
   */
  void
  createPipeline(vk::Device device, const char *vertShaderPath,
                 const char *fragShaderPath, vk::RenderPass renderPass,
                 vk::DescriptorSetLayout descriptorLayout, vk::PushConstantRange pushConstantRange,
                 std::deque<std::function<void(vk::Device)>> &deletionQueue);

  /**
   * @brief Initializes compute pipeline workflow
   */
  void
  createPipeline(vk::Device device, const char *shaderPath,
                 vk::DescriptorSetLayout descriptorLayout,vk::PushConstantRange pushConstantRange,
                 std::deque<std::function<void(vk::Device)>> &deletionQueue);

  ~Pipeline() = default;

private:
  /**
   * @brief Builds the pipeline
   * @param device Logical device
   * @param swapchainFormat the swapchain image format
   * @param vertShader compiled SPV vertex shader
   */

  void
  buildPipeline(vk::Device device, const char *vertShaderPath,
                const char *fragShaderPath, vk::RenderPass renderPass,
                vk::DescriptorSetLayout descriptorLayout,vk::PushConstantRange pushConstantRange,
                std::deque<std::function<void(vk::Device)>> &deletionQueue);

  /**
   * @brief Builds the pipeline
   * @param device Logical device
   * @param swapchainFormat the swapchain image format
   * @param vertShader compiled SPV vertex shader
   */
  void
  buildPipeline(vk::Device device, const char *shaderPath,
                vk::DescriptorSetLayout descriptorLayout,vk::PushConstantRange pushConstantRange,
                std::deque<std::function<void(vk::Device)>> &deletionQueue);

  /**
   * @brief Reads the content of the shader file
   * @param shaderFile SPV file to be read
   * @return vector of bytes
   */

  std::vector<char> readShader(const char *shaderFile);

  /**
   * @brief Create shader modules
   * @param code Shade SPV bytes
   * @param device Logical device
   * @return vk::ShaderModule Result shader module
   */
  vk::ShaderModule createShaderModule(const std::vector<char> &code,
                                      vk::Device device);

public:
  /**
   * @brief Graphics FillPipeline
   */
  vk::Pipeline pipeline;

  /**
   * @brief Graphics pipeline layout
   */
  vk::PipelineLayout layout;

};

#endif
