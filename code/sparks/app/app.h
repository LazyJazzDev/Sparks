#pragma once
#include "sparks/app/app_gui_renderer.h"
#include "sparks/app/app_settings.h"
#include "sparks/app/camera_controller.h"
#include "sparks/asset_manager/asset_manager.h"
#include "sparks/assets/assets.h"
#include "sparks/renderer/renderer.h"
#include "sparks/scene/scene.h"

namespace sparks {

class Application {
 public:
  Application(const AppSettings &settings);
  ~Application();
  void Run();
  void HeadlessMainLoop();

  vulkan::Core *Core() {
    return core_.get();
  }

 private:
  void OnInit();
  void OnUpdate();
  void OnRender();
  void OnClose();

  void CreateFrameImage(uint32_t width, uint32_t height);
  void CreateFrameImage();
  void CreateImGuiManager();
  void CreateAssetManager();
  void CreateRenderer();
  void CreateGuiRenderer();
  void RegisterInteractions();

  void LoadScene();

  void DestroyFrameImage();
  void DestroyImGuiManager();
  void DestroyAssetManager();
  void DestroyRenderer();
  void DestroyGuiRenderer();

  void ImGui();
  ImVec2 ImGuiSettingsWindow();
  ImVec2 ImGuizmoWindow();
  ImVec2 ImGuiStatisticWindow(ImVec2 window_pos);
  void CaptureMouseRelatedData();

  AppSettings settings_{};
  GLFWwindow *window_{};

  std::unique_ptr<vulkan::Core> core_;
  std::unique_ptr<vulkan::Image> frame_image_;

  std::unique_ptr<vulkan::ImGuiManager> imgui_manager_;

  std::unique_ptr<AssetManager> asset_manager_;
  std::unique_ptr<Renderer> renderer_;
  std::unique_ptr<Film> film_;
  std::unique_ptr<RayTracingFilm> raytracing_film_;
  std::unique_ptr<Scene> scene_;
  std::unique_ptr<CameraController> camera_controller_;

  std::unique_ptr<AppGuiRenderer> gui_renderer_;

  bool output_raytracing_result_{false};

  int window_width_;
  int window_height_;
  int framebuffer_width_;
  int framebuffer_height_;
  int cursor_x_;
  int cursor_y_;
  uint32_t hovering_instances_[2];
  uint32_t selected_instances_[2]{0xfffffffeu, 0xfffffffeu};
  uint32_t pre_selected_instances[2]{0xffffffffu, 0xffffffffu};
  glm::vec4 hovering_color_;

  bool reset_accumulated_buffer_{false};

  Material editing_material_;
  glm::mat4 editing_transform_{1.0f};
  SceneSettings editing_scene_settings_;
  EnvMapSettings editing_envmap_settings_;
  bool render_settings_changed_{false};
};

}  // namespace sparks
