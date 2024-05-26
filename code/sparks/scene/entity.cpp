#include "sparks/scene/entity.h"

#include "sparks/renderer/renderer.h"
#include "sparks/scene/scene.h"

namespace sparks {
Entity::Entity(Scene *scene) : scene_(scene) {
  descriptor_sets_.resize(scene->Renderer()->Core()->MaxFramesInFlight());
  for (auto &descriptor_set : descriptor_sets_) {
  }
}

vulkan::Core *Entity::Core() const {
  return scene_->Renderer()->Core();
}
}  // namespace sparks
