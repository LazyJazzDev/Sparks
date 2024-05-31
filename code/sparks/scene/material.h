#pragma once
#include "sparks/scene/scene_utils.h"

namespace sparks {
struct Material {
  glm::vec3 color{1.0f};
  float alpha{1.0f};
  glm::vec3 normal{0.5f, 0.5f, 1.0f};
  float padding0{0.0f};
};

}  // namespace sparks
