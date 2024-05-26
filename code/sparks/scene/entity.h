#pragma once
#include "sparks/scene/scene_utils.h"

namespace sparks {
class Entity {
 public:
  Entity(Scene *scene);

  vulkan::Core *Core() const;

 private:
  Scene *scene_;
  std::vector<std::unique_ptr<vulkan::DescriptorSet>> descriptor_sets_;
};
}  // namespace sparks
