#pragma once
#include "glm/glm.hpp"

namespace sparks {

struct SceneSettings {
  glm::mat4 projection;      // camera_to_clip
  glm::mat4 inv_projection;  // inverse_projection
  glm::mat4 view;            // world_to_camera
  glm::mat4 inv_view;        // camera_to_world
  float gamma{2.2f};
  float exposure{1.0f};
  float persistence{0.9f};
  float padding[13];
};  // need align to 64(0x40) byte

}  // namespace sparks
