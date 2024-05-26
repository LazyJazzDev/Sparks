#pragma once
#include "sparks/asset_manager/asset_manager.h"
#include "sparks/scene/entity.h"
#include "sparks/scene/material.h"
#include "sparks/scene/scene_settings.h"
#include "sparks/scene/scene_utils.h"

namespace sparks {
class Scene {
 public:
  Scene(class Renderer *renderer,
        class AssetManager *asset_manager,
        int max_entities);

  class Renderer *Renderer() {
    return renderer_;
  }

  class AssetManager *AssetManager() {
    return asset_manager_;
  }

 private:
  class Renderer *renderer_{};
  class AssetManager *asset_manager_{};
  std::unique_ptr<vulkan::DescriptorPool> descriptor_pool_;
  std::unique_ptr<vulkan::DynamicBuffer<SceneSettings>> scene_settings_buffer_;
  std::vector<std::unique_ptr<vulkan::DescriptorSet>> descriptor_sets_;
};
}  // namespace sparks
