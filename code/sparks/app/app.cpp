#include "sparks/app/app.h"

namespace sparks {

Application::Application(const AppSettings &settings) : settings_(settings) {
  if (!settings.headless) {
    if (!glfwInit()) {
      throw std::runtime_error("Failed to initialize GLFW");
    }
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window_ = glfwCreateWindow(settings.frame_width, settings.frame_height,
                               "Sparks", nullptr, nullptr);
    if (!window_) {
      glfwTerminate();
      throw std::runtime_error("Failed to create GLFW window");
    }
  }

  vulkan::CoreSettings core_settings;
  core_settings.enable_ray_tracing = true;
  core_settings.window = window_;
  core_settings.max_frames_in_flight = 2;
  core_ = std::make_unique<vulkan::Core>(core_settings);
}

Application::~Application() {
  core_.reset();
  if (!settings_.headless) {
    glfwDestroyWindow(window_);
    glfwTerminate();
  }
}

void Application::Run() {
  OnInit();
  if (settings_.headless) {
    HeadlessMainLoop();
    return;
  } else {
    while (!glfwWindowShouldClose(window_)) {
      OnUpdate();
      OnRender();
      glfwPollEvents();
    }
  }
  core_->Device()->WaitIdle();
  OnClose();
}

void Application::HeadlessMainLoop() {
}

void Application::OnInit() {
  CreateFrameImage();
  CreateImGuiManager();
  CreateAssetManager();
  CreateRenderer();
  LoadScene();
}

void Application::OnClose() {
  DestroyRenderer();
  DestroyAssetManager();
  DestroyImGuiManager();
  DestroyFrameImage();
}

void Application::OnUpdate() {
  auto current_time = std::chrono::high_resolution_clock::now();
  static auto last_time = current_time;
  float delta_time = std::chrono::duration<float, std::chrono::seconds::period>(
                         current_time - last_time)
                         .count();

  scene_->Update(delta_time);

  imgui_manager_->BeginFrame();
  ImGui::ShowDemoWindow();
  asset_manager_->ImGui();
  imgui_manager_->EndFrame();

  core_->TransferCommandPool()->SingleTimeCommands(
      core_->TransferQueue(), [&](VkCommandBuffer cmd_buffer) {
        scene_->SyncData(cmd_buffer, core_->CurrentFrame());
      });
}

void Application::OnRender() {
  core_->BeginFrame();
  VkCommandBuffer cmd_buffer = core_->CommandBuffer()->Handle();
  frame_image_->ClearColor(cmd_buffer, {0.0f, 0.0f, 0.0f, 1.0f});
  vulkan::TransitImageLayout(
      cmd_buffer, frame_image_->Handle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_TRANSFER_BIT,
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
      VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
      VK_IMAGE_ASPECT_COLOR_BIT);

  renderer_->RenderScene(cmd_buffer, film_.get(), scene_.get());

  imgui_manager_->Render(cmd_buffer);

  vulkan::TransitImageLayout(
      cmd_buffer, frame_image_->Handle(), VK_IMAGE_LAYOUT_GENERAL,
      VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
      VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
      VK_ACCESS_TRANSFER_READ_BIT, VK_IMAGE_ASPECT_COLOR_BIT);

  //  core_->OutputFrame(frame_image_.get());
  core_->OutputFrame(film_->intensity_image.get());
  core_->EndFrame();
}

void Application::CreateFrameImage() {
  int width, height;
  glfwGetFramebufferSize(window_, &width, &height);
  CreateFrameImage(width, height);
  core_->FrameSizeEvent().RegisterCallback(
      [this](uint32_t width, uint32_t height) {
        CreateFrameImage(width, height);
      });
}

void Application::CreateFrameImage(uint32_t width, uint32_t height) {
  core_->Device()->WaitIdle();
  if (frame_image_) {
    frame_image_->Resize(VkExtent2D{width, height});
  } else {
    core_->Device()->CreateImage(
        VK_FORMAT_B8G8R8A8_UNORM,
        VkExtent2D{static_cast<uint32_t>(width), static_cast<uint32_t>(height)},
        &frame_image_);
  }
}

void Application::DestroyFrameImage() {
  frame_image_.reset();
}

void Application::CreateImGuiManager() {
  imgui_manager_ = std::make_unique<vulkan::ImGuiManager>(
      core_.get(), frame_image_.get(),
      FindAssetsFile("fonts/NotoSans-Regular.ttf").c_str(), 20.0f);
}

void Application::DestroyImGuiManager() {
  imgui_manager_.reset();
}

void Application::CreateAssetManager() {
  asset_manager_ = std::make_unique<AssetManager>(core_.get());
}

void Application::DestroyAssetManager() {
  asset_manager_.reset();
}

void Application::CreateRenderer() {
  renderer_ = std::make_unique<Renderer>(core_.get());
  renderer_->CreateFilm(core_->Swapchain()->Extent().width,
                        core_->Swapchain()->Extent().height, &film_);
  core_->FrameSizeEvent().RegisterCallback(
      [this](int width, int height) { film_->Resize(width, height); });
  renderer_->CreateScene(asset_manager_.get(), 2, &scene_);
}

void Application::DestroyRenderer() {
  scene_.reset();
  film_.reset();
  renderer_.reset();
}

void Application::LoadScene() {
  auto asset_manager = scene_->AssetManager();
  scene_->Camera()->GetPosition() = glm::vec3{0.0f, 0.0f, 5.0f};
  int entity_id = scene_->CreateEntity();
  auto entity = scene_->GetEntity(entity_id);
}

}  // namespace sparks
