#pragma once
#include "sparks/scene/scene_utils.h"

namespace sparks {
struct Material {
  glm::vec3 color{1.0f};
  float alpha{1.0f};
  glm::vec4 detail_scale_offset{10.0f, 10.0f, 0.0f, 0.0f};
};

}  // namespace sparks
