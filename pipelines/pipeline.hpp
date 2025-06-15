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
   * @brief Initializes the pipeline workflow
   */
  virtual void createPipeline(
      vk::Device device, const char *vertShaderPath, const char *fragShaderPath,
      vk::RenderPass renderPass, vk::DescriptorSetLayout descriptorLayout,
      std::deque<std::function<void(vk::Device)>> &deletionQueue) = 0;
  virtual ~Pipeline() = default;

protected:
  /**
   * @brief Builds the pipeline
   * @param device Logical device
   * @param swapchainFormat the swapchain image format
   * @param vertShader compiled SPV vertex shader
   * @param fragShader compiled SPV fragment shader
   * @param vertexType pointer to unique vertex type
   */

  virtual void
  buildPipeline(vk::Device device, const char *vertShaderPath,
                const char *fragShaderPath, vk::RenderPass renderPass,
                vk::DescriptorSetLayout descriptorLayout,
                std::deque<std::function<void(vk::Device)>> &deletionQueue) = 0;

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

  /**
   * @brief Pipeline name
   */
  std::string name;
};

#endif
