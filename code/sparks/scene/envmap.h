#pragma once
#include "sparks/scene/scene_utils.h"

namespace sparks {

struct EnvMapSettings {
  float offset{0.0f};
  float exposure{1.0f};
  int reflect{1};
};

class EnvMap {
 public:
  EnvMap(Scene *scene);
  ~EnvMap();

  EnvMapSettings &Settings() {
    return settings_;
  }

  const EnvMapSettings &Settings() const {
    return settings_;
  }

  vulkan::DescriptorSet *DescriptorSet(int frame_id) {
    return descriptor_sets_[frame_id].get();
  }

  void SetEnvmapTexture(uint32_t envmap_id);

  void Update();

  void Sync(VkCommandBuffer cmd_buffer, int frame_id);

 private:
  Scene *scene_{nullptr};
  EnvMapSettings settings_{};
  std::unique_ptr<vulkan::DynamicBuffer<EnvMapSettings>>
      envmap_settings_buffer_{};
  std::vector<std::unique_ptr<vulkan::DescriptorSet>> descriptor_sets_{};
};
}  // namespace sparks
