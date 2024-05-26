#include "sparks/app/camera_controller.h"

#include "glm/gtc/matrix_transform.hpp"

namespace sparks {

CameraController::CameraController(vulkan::Core *core, Camera *camera)
    : core_(core), camera_(camera) {
  double xpos, ypos;
  glfwGetCursorPos(core_->Surface()->Window(), &xpos, &ypos);
  last_x = xpos;
  last_y = ypos;
}

void CameraController::Update(float delta_time) {
  auto camera_to_world = camera_->GetInverseView();
  GLFWwindow *window = core_->Surface()->Window();
  glm::vec3 x_axis{camera_to_world[0]};
  glm::vec3 y_axis{camera_to_world[1]};
  glm::vec3 z_axis{camera_to_world[2]};
  double xpos, ypos;
  glfwGetCursorPos(core_->Surface()->Window(), &xpos, &ypos);
  double diff_x = xpos - last_x;
  double diff_y = ypos - last_y;
  last_x = xpos;
  last_y = ypos;

  glm::vec2 delta_angle{diff_x, diff_y};
  delta_angle *= glm::radians(0.2f);
  if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT)) {
    auto &euler_angles = camera_->GetEulerAngles();
    euler_angles.x -= delta_angle.y;
    euler_angles.y -= delta_angle.x;
    euler_angles.x = glm::clamp(euler_angles.x, -glm::half_pi<float>(),
                                glm::half_pi<float>());
    euler_angles.y = glm::mod(euler_angles.y, glm::two_pi<float>());
  }

  glm::vec3 move_dir{0.0f};
  if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
    move_dir -= z_axis;
  }
  if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
    move_dir += z_axis;
  }
  if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
    move_dir -= x_axis;
  }
  if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
    move_dir += x_axis;
  }
  if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
    move_dir -= y_axis;
  }
  if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
    move_dir += y_axis;
  }
  camera_->GetPosition() += move_dir * 3.0f * delta_time;
}

}  // namespace sparks