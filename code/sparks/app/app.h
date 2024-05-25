#pragma once
#include "sparks/app/app_settings.h"

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

  void DestroyFrameImage();
  void DestroyImGuiManager();

  AppSettings settings_{};
  GLFWwindow *window_{};

  std::unique_ptr<vulkan::Core> core_;
  std::unique_ptr<vulkan::Image> frame_image_;

  std::unique_ptr<vulkan::ImGuiManager> imgui_manager_;
};

}  // namespace sparks
