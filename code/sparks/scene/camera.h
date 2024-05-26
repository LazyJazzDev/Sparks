#pragma once
#include "sparks/scene/scene_utils.h"

namespace sparks {
class Camera {
 public:
  Camera();
  ~Camera();

  glm::mat4 GetInverseView() const;
  glm::mat4 GetView() const;
  glm::mat4 GetProjection(float aspect) const;

  glm::vec3 &GetPosition() {
    return position_;
  }

  const glm::vec3 &GetPosition() const {
    return position_;
  }

  glm::vec3 &GetEulerAngles() {
    return euler_angles_;
  }  // pitch, yaw, roll

  const glm::vec3 &GetEulerAngles() const {
    return euler_angles_;
  }  // pitch, yaw, roll

  float &GetFov() {
    return fov_;
  }

  const float &GetFov() const {
    return fov_;
  }

  float &GetNear() {
    return near_;
  }

  const float &GetNear() const {
    return near_;
  }

  float &GetFar() {
    return far_;
  }

  const float &GetFar() const {
    return far_;
  }

 private:
  glm::vec3 euler_angles_{0.0f, 0.0f, 0.0f};
  glm::vec3 position_{0.0f, 0.0f, 0.0f};
  float fov_{glm::radians(45.0f)};
  float near_{0.1f};
  float far_{1000.0f};
};
}  // namespace sparks
