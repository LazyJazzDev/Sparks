#pragma once
#include "sparks/asset_manager/asset_manager_utils.h"

namespace sparks {
struct TextureAsset {
  std::unique_ptr<vulkan::Image> image_;
  std::string name_;
};
}  // namespace sparks
