#pragma once
#include "sparks/scene/material.h"
#include "sparks/scene/scene_utils.h"

namespace sparks {

struct EntityMetadata {
  glm::mat4 transform{1.0f};
  uint32_t entity_id{0};
  uint32_t mesh_id{0};
  uint32_t albedo_texture_id{0};
  uint32_t albedo_detail_texture_id{0};
  glm::vec4 detail_scale_offset{10.0f, 10.0f, 0.0f, 0.0f};
};

class Entity {
 public:
  Entity(Scene *scene);
  ~Entity();

  vulkan::Core *Core() const;

  uint32_t MeshId() const {
    return metadata_.mesh_id;
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

  EntityMetadata GetTranslatedMetadata() const;

  void Update();
  void Sync(VkCommandBuffer cmd_buffer, int frame_id);

 private:
  friend Scene;

  Scene *scene_;
  Material material_{};
  EntityMetadata metadata_{};
  std::vector<std::unique_ptr<vulkan::DescriptorSet>> descriptor_sets_;
  std::unique_ptr<vulkan::DynamicBuffer<EntityMetadata>> metadata_buffer_;
  std::unique_ptr<vulkan::DynamicBuffer<Material>> material_buffer_;
};
}  // namespace sparks
