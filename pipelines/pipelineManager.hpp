#ifndef PIPELINE_MANAGER_HPP
#define PIPELINE_MANAGER_HPP
#define VULKAN_HPP_NO_EXCEPTIONS
#include <deque>
#include <functional>
#include <string>
#include <unordered_map>
#include <vulkan/vulkan.hpp>


enum class PipelineType { Fill, WireFrame };

struct PipelineInstance {
  PipelineType type;
  std::string vertShaderSrc;
  std::string fragShaderSrc;

  bool operator==(const PipelineInstance &other) const {
    return type == other.type && vertShaderSrc == other.vertShaderSrc &&
           fragShaderSrc == other.fragShaderSrc;
  }
};

template <> struct std::hash<PipelineInstance> {
  std::size_t operator()(const PipelineInstance &p) const noexcept {
    std::size_t h1 = std::hash<std::string>{}(p.vertShaderSrc);
    std::size_t h2 = std::hash<std::string>{}(p.fragShaderSrc);
    std::size_t h3 = std::hash<int>{}(static_cast<int>(p.type));
    return h3 ^ (h1 << 1) ^ (h2 << 2);
  }
};

class PipelineManager {
public:
  PipelineManager() = default;
  PipelineManager(vk::Device logicalDevice,  vk::RenderPass renderPass);
  std::pair<vk::Pipeline,vk::PipelineLayout> loadPipeline(PipelineInstance);

  /**
   * @brief Creates the descriptor sets
   */
  void createDescriptorSetLayout();

  void createDescriptorPool(int framesInFlight);

  void cleanUp();

  vk::DescriptorSetLayout getDescriptorSetLayout() const ;
  vk::DescriptorPool getDescriptorPool() const;


private:
  std::unordered_map<PipelineInstance, std::pair<vk::Pipeline,vk::PipelineLayout>> pipelineMap;
  vk::Device logicalDevice;
  vk::RenderPass renderPass;
  vk::DescriptorSetLayout descriptorSetLayout;
  vk::DescriptorPool descriptorPool;
  std::deque<std::function<void(vk::Device)>> deletionQueue;
};

#endif
