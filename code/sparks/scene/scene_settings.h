#pragma once
#include "glm/glm.hpp"

namespace sparks {

struct SceneSettings {
  glm::mat4 projection;      // camera_to_clip
  glm::mat4 inv_projection;  // inverse_projection
  glm::mat4 view;            // world_to_camera
  glm::mat4 inv_view;        // camera_to_world
};

}  // namespace sparks
