#pragma once
#include <utility>

#include "sparks/asset_manager/asset_manager.h"
#include "sparks/scene/camera.h"
#include "sparks/scene/entity.h"
#include "sparks/scene/envmap.h"
#include "sparks/scene/material.h"
#include "sparks/scene/scene_settings.h"
#include "sparks/scene/scene_utils.h"

namespace sparks {
class Scene {
 public:
  Scene(class Renderer *renderer,
        class AssetManager *asset_manager,
        int max_entities);
  ~Scene();

  class Renderer *Renderer() {
    return renderer_;
  }

  class AssetManager *AssetManager() {
    return asset_manager_;
  }

  class Camera *Camera() {
    return &camera_;
  }

  const class Camera *Camera() const {
    return &camera_;
  }

  vulkan::DescriptorPool *DescriptorPool() {
    return descriptor_pool_.get();
  }

  int CreateEntity(Entity **pp_entity = nullptr);
  Entity *GetEntity(uint32_t id) const {
    return entities_.at(id).get();
  }

  EnvMap *GetEnvMap() const {
    return envmap_.get();
  }

  void SetUpdateCallback(std::function<void(Scene *, float)> callback) {
    update_callback_ = std::move(callback);
  }

  void Update(float delta_time);

  void SyncData(VkCommandBuffer cmd_buffer, int frame_id);

  void DrawEnvmap(VkCommandBuffer cmd_buffer, int frame_id);

  void DrawEntities(VkCommandBuffer cmd_buffer, int frame_id);

  VkDescriptorSet SceneSettingsDescriptorSet(int frame_id) const {
    return descriptor_sets_[frame_id]->Handle();
  }

 private:
  void UpdateDynamicBuffers();

  class Renderer *renderer_{};
  class AssetManager *asset_manager_{};
  std::unique_ptr<vulkan::DescriptorPool> descriptor_pool_{};

  std::unique_ptr<vulkan::DynamicBuffer<SceneSettings>>
      scene_settings_buffer_{};
  std::vector<std::unique_ptr<vulkan::DescriptorSet>> descriptor_sets_{};

  std::map<uint32_t, std::unique_ptr<Entity>> entities_{};
  uint32_t next_entity_id_{};

  std::unique_ptr<EnvMap> envmap_{};

  class Camera camera_ {};

  std::function<void(Scene *, float)> update_callback_{};
};
}  // namespace sparks
