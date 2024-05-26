#pragma once
#include "sparks/asset_manager/asset_manager.h"
#include "sparks/renderer/film.h"
#include "sparks/renderer/renderer_utils.h"

namespace sparks {
class Renderer {
 public:
  Renderer(vulkan::Core *core);
  ~Renderer();

  vulkan::Core *Core() {
    return core_;
  }

  int CreateScene(AssetManager *asset_manager,
                  double_ptr<Scene> *pp_scene,
                  int max_entities);

  vulkan::DescriptorSetLayout *SceneDescriptorSetLayout() {
    return scene_descriptor_set_layout_.get();
  }

  vulkan::DescriptorSetLayout *EntityDescriptorSetLayout() {
    return entity_descriptor_set_layout_.get();
  }

  vulkan::RenderPass *RenderPass() {
    return render_pass_.get();
  }

  vulkan::Sampler *DefaultSampler() {
    return sampler_.get();
  }

 private:
  void CreateDescriptorSetLayouts();
  void CreatePipelines();

  void DestroyDescriptorSetLayouts();
  void DestroyPipelines();

  vulkan::Core *core_;
  std::unique_ptr<vulkan::DescriptorSetLayout> scene_descriptor_set_layout_;
  std::unique_ptr<vulkan::DescriptorSetLayout> entity_descriptor_set_layout_;
  std::unique_ptr<vulkan::RenderPass> render_pass_;
  std::unique_ptr<vulkan::PipelineLayout> pipeline_layout_;
  std::unique_ptr<vulkan::ShaderModule> vertex_shader_;
  std::unique_ptr<vulkan::ShaderModule> fragment_shader_;
  std::unique_ptr<vulkan::Pipeline> pipeline_;
  std::unique_ptr<vulkan::Sampler> sampler_;
};
}  // namespace sparks
