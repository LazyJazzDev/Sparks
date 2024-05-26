#pragma once
#include "glm/glm.hpp"

namespace sparks {

struct SceneSettings {
  glm::mat4 projection;
  glm::mat4 inv_projection;
  glm::mat4 world_to_camera;
  glm::mat4 camera_to_world;
};

}  // namespace sparks
