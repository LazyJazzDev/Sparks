#include "sparks/renderer/renderer.h"

#include "sparks/scene/scene.h"

namespace sparks {

Renderer::Renderer(vulkan::Core *core) : core_(core) {
  CreateDescriptorSetLayouts();
  CreatePipelines();
}

Renderer::~Renderer() {
  DestroyPipelines();
  DestroyDescriptorSetLayouts();
}

void Renderer::CreateDescriptorSetLayouts() {
  core_->Device()->CreateDescriptorSetLayout(
      {{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT,
        nullptr}},
      &scene_descriptor_set_layout_);
}

void Renderer::DestroyDescriptorSetLayouts() {
  scene_descriptor_set_layout_.reset();
}

void Renderer::CreatePipelines() {
}

void Renderer::DestroyPipelines() {
}

int Renderer::CreateScene(AssetManager *asset_manager,
                          double_ptr<Scene> *pp_scene,
                          int max_entities) {
  pp_scene->construct(this, asset_manager, max_entities);
  return 0;
}
}  // namespace sparks
