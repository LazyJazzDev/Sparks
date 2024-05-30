#pragma once

#include "sparks/asset_manager/asset_manager.h"
#include "sparks/renderer/film.h"
#include "sparks/renderer/raytracing_film.h"
#include "sparks/renderer/renderer_utils.h"
#include "sparks/scene/scene.h"

namespace sparks {
class Renderer {
 public:
  Renderer(class AssetManager *asset_manager);

  ~Renderer();

  vulkan::Core *Core() {
    return core_;
  }

  class AssetManager *AssetManager() {
    return asset_manager_;
  }

  int CreateScene(int max_entities, double_ptr<class Scene> pp_scene);

  vulkan::DescriptorSetLayout *SceneDescriptorSetLayout() {
    return scene_descriptor_set_layout_.get();
  }

  vulkan::DescriptorSetLayout *EnvmapDescriptorSetLayout() {
    return envmap_descriptor_set_layout_.get();
  }

  vulkan::DescriptorSetLayout *EntityDescriptorSetLayout() {
    return entity_descriptor_set_layout_.get();
  }

  vulkan::DescriptorSetLayout *RayTracingDescriptorSetLayout() {
    return raytracing_descriptor_set_layout_.get();
  }

  vulkan::DescriptorSetLayout *RayTracingFilmDescriptorSetLayout() {
    return raytracing_film_descriptor_set_layout_.get();
  }

  vulkan::Pipeline *EntityPipeline() {
    return entity_pipeline_.get();
  }

  vulkan::Pipeline *EnvmapPipeline() {
    return envmap_pipeline_.get();
  }

  vulkan::PipelineLayout *EntityPipelineLayout() {
    return entity_pipeline_layout_.get();
  }

  vulkan::PipelineLayout *EnvmapPipelineLayout() {
    return envmap_pipeline_layout_.get();
  }

  vulkan::RenderPass *RenderPass() {
    return render_pass_.get();
  }

  vulkan::Sampler *DefaultSampler() {
    return sampler_.get();
  }

  int CreateFilm(uint32_t width, uint32_t height, double_ptr<Film> pp_film);

  int CreateRayTracingFilm(uint32_t width,
                           uint32_t height,
                           double_ptr<RayTracingFilm> pp_film);

  void RenderScene(VkCommandBuffer cmd_buffer,
                   Film *film,
                   sparks::Scene *scene);

  void RenderSceneRayTracing(VkCommandBuffer cmd_buffer,
                             RayTracingFilm *film,
                             sparks::Scene *scene);

 private:
  void CreateDescriptorSetLayouts();

  void CreateRenderPass();

  void CreateEnvmapPipeline();

  void CreateEntityPipeline();

  void CreateRayTracingPipeline();

  void DestroyDescriptorSetLayouts();

  void DestroyRenderPass();

  void DestroyEnvmapPipeline();

  void DestroyEntityPipeline();

  void DestroyRayTracingPipeline();

  vulkan::Core *core_{};
  class AssetManager *asset_manager_{};
  std::unique_ptr<vulkan::DescriptorSetLayout> scene_descriptor_set_layout_;
  std::unique_ptr<vulkan::RenderPass> render_pass_;

  std::unique_ptr<vulkan::DescriptorSetLayout> entity_descriptor_set_layout_;
  std::unique_ptr<vulkan::PipelineLayout> entity_pipeline_layout_;
  std::unique_ptr<vulkan::ShaderModule> entity_vertex_shader_;
  std::unique_ptr<vulkan::ShaderModule> entity_fragment_shader_;
  std::unique_ptr<vulkan::Pipeline> entity_pipeline_;

  std::unique_ptr<vulkan::DescriptorSetLayout> envmap_descriptor_set_layout_;
  std::unique_ptr<vulkan::PipelineLayout> envmap_pipeline_layout_;
  std::unique_ptr<vulkan::ShaderModule> envmap_vertex_shader_;
  std::unique_ptr<vulkan::ShaderModule> envmap_fragment_shader_;
  std::unique_ptr<vulkan::Pipeline> envmap_pipeline_;

  std::unique_ptr<vulkan::DescriptorSetLayout>
      raytracing_descriptor_set_layout_;
  std::unique_ptr<vulkan::DescriptorSetLayout>
      raytracing_film_descriptor_set_layout_;
  std::unique_ptr<vulkan::PipelineLayout> raytracing_pipeline_layout_;
  std::unique_ptr<vulkan::ShaderModule> raytracing_raygen_shader_;
  std::unique_ptr<vulkan::ShaderModule> raytracing_miss_shader_;
  std::unique_ptr<vulkan::ShaderModule> raytracing_closest_hit_shader_;
  std::unique_ptr<vulkan::Pipeline> raytracing_pipeline_;
  std::unique_ptr<vulkan::ShaderBindingTable> raytracing_sbt_;

  std::unique_ptr<vulkan::Sampler> sampler_;
};
}  // namespace sparks
