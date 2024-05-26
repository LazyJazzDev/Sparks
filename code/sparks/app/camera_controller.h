#pragma once
#include "sparks/scene/camera.h"
#include "sparks/utils/utils.h"

namespace sparks {
class CameraController {
 public:
  CameraController(vulkan::Core *core, Camera *camera);
  void Update(float delta_time);

 private:
  vulkan::Core *core_;
  Camera *camera_;

  double last_x, last_y;
};
}  // namespace sparks
