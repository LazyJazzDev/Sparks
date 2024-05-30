#pragma once
#include "sparks/scene/material.h"
#include "sparks/scene/scene_utils.h"

namespace sparks {

struct EntityMetadata {
  glm::mat4 transform{1.0f};
  uint32_t entity_id{0};
  uint32_t albedo_map_id{0};
  uint32_t albedo_detail_map_id{0};
  uint32_t padding0{0xccccccccu};
};

class Entity {
 public:
  Entity(Scene *scene);
  ~Entity();

  vulkan::Core *Core() const;

  void SetMesh(uint32_t mesh_id);

  uint32_t MeshId() const {
    return mesh_id_;
  }

  Material GetMaterial() const {
    return material_;
  }

  vulkan::DescriptorSet *DescriptorSet(int frame_id) {
    return descriptor_sets_[frame_id].get();
  }

  glm::mat4 GetTransform() const {
    return metadata_.transform;
  }

  void SetAlbedoTexture(uint32_t texture_id);
  void SetAlbedoDetailTexture(uint32_t texture_id);

  void Update();
  void Sync(VkCommandBuffer cmd_buffer, int frame_id);

 private:
  friend Scene;
  Scene *scene_;
  uint32_t mesh_id_{};
  Material material_{};
  EntityMetadata metadata_{};
  std::vector<std::unique_ptr<vulkan::DescriptorSet>> descriptor_sets_;
  std::unique_ptr<vulkan::DynamicBuffer<EntityMetadata>> metadata_buffer_;
  std::unique_ptr<vulkan::DynamicBuffer<Material>> material_buffer_;
};
}  // namespace sparks
