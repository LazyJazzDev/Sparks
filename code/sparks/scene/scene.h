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
  Scene(struct Renderer *renderer, int max_entities);

  ~Scene();

  class Renderer *Renderer() {
    return renderer_;
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

  VkDescriptorSet RayTracingDescriptorSet(int frame_id) const {
    return raytracing_descriptor_sets_[frame_id]->Handle();
  }

  int SetEnvmapSettings(const EnvMapSettings &settings);

  int SetEntityTransform(uint32_t entity_id, const glm::mat4 &transform);

  int GetEntityTransform(uint32_t entity_id, glm::mat4 &transform) const;

  int SetEntityMaterial(uint32_t entity_id, const Material &material);

  int GetEntityMaterial(uint32_t entity_id, Material &material) const;

  int SetEntityAlbedoTexture(uint32_t entity_id, uint32_t texture_id);

  int SetEntityAlbedoDetailTexture(uint32_t entity_id, uint32_t texture_id);

  int SetEntityMesh(uint32_t entity_id, uint32_t mesh_id);

 private:
  void UpdateDynamicBuffers();
  void UpdateTopLevelAccelerationStructure();
  void UpdateDescriptorSetBindings();

  class Renderer *renderer_{};

  std::unique_ptr<vulkan::DescriptorPool> descriptor_pool_{};

  std::unique_ptr<vulkan::DynamicBuffer<SceneSettings>>
      scene_settings_buffer_{};
  std::vector<std::unique_ptr<vulkan::DescriptorSet>> descriptor_sets_{};

  std::map<uint32_t, std::unique_ptr<Entity>> entities_{};
  uint32_t next_entity_id_{};

  std::unique_ptr<EnvMap> envmap_{};

  class Camera camera_ {};

  std::function<void(Scene *, float)> update_callback_{};

  std::vector<std::unique_ptr<vulkan::DescriptorSet>>
      raytracing_descriptor_sets_{};

  std::unique_ptr<vulkan::AccelerationStructure> top_level_as_{};

  std::unique_ptr<vulkan::DynamicBuffer<Material>> entity_material_buffer_{};
  std::unique_ptr<vulkan::DynamicBuffer<EntityMetadata>>
      entity_metadata_buffer_{};
};
}  // namespace sparks
