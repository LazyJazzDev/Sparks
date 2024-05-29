#pragma once
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

  void LoadScene();

  void DestroyFrameImage();
  void DestroyImGuiManager();
  void DestroyAssetManager();
  void DestroyRenderer();

  void ImGui();

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

  int output_frame_{0};
};

}  // namespace sparks
