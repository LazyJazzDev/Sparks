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

  glm::vec3 GetPosition() const {
    return position_;
  }

  void SetPosition(const glm::vec3 &position) {
    position_ = position;
  }

  glm::vec3 GetEulerAngles() const {
    return euler_angles_;
  }  // pitch, yaw, roll

  void SetEulerAngles(const glm::vec3 &euler_angles) {
    euler_angles_ = euler_angles;
  }

  float GetFov() const {
    return fov_;
  }

  void SetFov(float fov) {
    fov_ = fov;
  }

  float GetNear() const {
    return near_;
  }

  void SetNear(float nearf) {
    near_ = nearf;
  }

  float GetFar() const {
    return far_;
  }

  void SetFar(float farf) {
    far_ = farf;
  }

 private:
  glm::vec3 euler_angles_{0.0f, 0.0f, 0.0f};
  glm::vec3 position_{0.0f, 0.0f, 0.0f};
  float fov_{glm::radians(45.0f)};
  float near_{0.1f};
  float far_{1000.0f};
};
}  // namespace sparks
