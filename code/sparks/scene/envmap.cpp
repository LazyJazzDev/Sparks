#include "sparks/scene/envmap.h"

#include "sparks/renderer/renderer.h"
#include "sparks/scene/scene.h"

namespace sparks {

EnvMap::EnvMap(Scene *scene) : scene_(scene) {
  envmap_settings_buffer_ =
      std::make_unique<vulkan::DynamicBuffer<EnvMapSettings>>(
          scene_->Renderer()->Core(), 1, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

  descriptor_sets_.resize(scene_->Renderer()->Core()->MaxFramesInFlight());

  auto asset_manager = scene_->Renderer()->AssetManager();

  for (size_t i = 0; i < descriptor_sets_.size(); ++i) {
    scene_->DescriptorPool()->AllocateDescriptorSet(
        scene_->Renderer()->EnvmapDescriptorSetLayout()->Handle(),
        &descriptor_sets_[i]);
    descriptor_sets_[i]->BindUniformBuffer(
        0, envmap_settings_buffer_->GetBuffer(i));
    auto texture = asset_manager->GetTexture(0);
    descriptor_sets_[i]->BindCombinedImageSampler(1, texture->image_.get());
  }
}

EnvMap::~EnvMap() {
  descriptor_sets_.clear();
  envmap_settings_buffer_.reset();
}

void EnvMap::Update() {
  envmap_settings_buffer_->At(0) = settings_;
}

void EnvMap::Sync(VkCommandBuffer cmd_buffer, int frame_id) {
  envmap_settings_buffer_->SyncData(cmd_buffer, frame_id);
}

void EnvMap::SetEnvmapTexture(uint32_t envmap_id) {
  scene_->Renderer()->Core()->Device()->WaitIdle();
  auto asset_manager = scene_->Renderer()->AssetManager();
  auto texture = asset_manager->GetTexture(envmap_id);
  for (size_t i = 0; i < descriptor_sets_.size(); ++i) {
    descriptor_sets_[i]->BindCombinedImageSampler(1, texture->image_.get());
  }
}

}  // namespace sparks
