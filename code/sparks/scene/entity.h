#pragma once
#include "sparks/scene/scene_utils.h"

namespace sparks {
class Entity {
 public:
  Entity(Scene *scene);

  vulkan::Core *Core() const;

  uint32_t MeshId() const {
    return mesh_id_;
  }

 private:
  Scene *scene_;
  uint32_t mesh_id_{};
  std::vector<std::unique_ptr<vulkan::DescriptorSet>> descriptor_sets_;
};
}  // namespace sparks
