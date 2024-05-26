#include "sparks/app/app.h"

#include "glm/gtc/matrix_transform.hpp"

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

  //  Texture skybox[5];
  //  skybox[0].LoadFromFile(FindAssetsFile("texture/terrain/SkyBox/SkyBox0.bmp"));
  //  skybox[1].LoadFromFile(FindAssetsFile("texture/terrain/SkyBox/SkyBox1.bmp"));
  //  skybox[2].LoadFromFile(FindAssetsFile("texture/terrain/SkyBox/SkyBox2.bmp"));
  //  skybox[3].LoadFromFile(FindAssetsFile("texture/terrain/SkyBox/SkyBox3.bmp"));
  //  skybox[4].LoadFromFile(FindAssetsFile("texture/terrain/SkyBox/SkyBox4.bmp"));
  //  Texture envmap = SkyBoxToEnvmap(
  //      {&skybox[0], &skybox[1], &skybox[2], &skybox[3], &skybox[4]}, 2048);
  //  envmap.SaveToFile("envmap.png");
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
  last_time = current_time;

  scene_->Update(delta_time);
  camera_controller_->Update(delta_time);

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

  camera_controller_ =
      std::make_unique<CameraController>(core_.get(), scene_->Camera());
}

void Application::DestroyRenderer() {
  camera_controller_.reset();
  scene_.reset();
  film_.reset();
  renderer_.reset();
}

void Application::LoadScene() {
  auto asset_manager = scene_->AssetManager();
  scene_->Camera()->GetPosition() = glm::vec3{0.0f, 0.0f, 5.0f};
  int entity_id = scene_->CreateEntity();
  auto entity = scene_->GetEntity(entity_id);
  auto envmap = scene_->GetEnvMap();

  Texture envmap_texture;
  envmap_texture.LoadFromFile(FindAssetsFile("texture/terrain/envmap.png"));
  auto envmap_id = asset_manager->LoadTexture(envmap_texture, "Envmap");
  envmap->SetEnvmapTexture(envmap_id);

  Texture terrain_texture;
  terrain_texture.LoadFromFile(
      FindAssetsFile("texture/terrain/terrain-texture3.bmp"));

  Texture terrain_detail_texture;
  terrain_detail_texture.LoadFromFile(
      FindAssetsFile("texture/terrain/detail.bmp"));
  auto terrain_texture_id =
      asset_manager->LoadTexture(terrain_texture, "TerrainTexture");
  auto terrain_detail_texture_id = asset_manager->LoadTexture(
      terrain_detail_texture, "TerrainDetailTexture");

  entity->SetAlbedoTexture(terrain_texture_id);
  entity->SetAlbedoDetailTexture(terrain_detail_texture_id);

  Texture heightmap_texture;
  heightmap_texture.LoadFromFile(
      FindAssetsFile("texture/terrain/heightmap.bmp"));
  Mesh terrain_mesh;
  terrain_mesh.LoadFromHeightMap(heightmap_texture, 1.0f, 0.2f, 0.0f);
  auto terrain_mesh_id = asset_manager->LoadMesh(terrain_mesh, "TerrainMesh");
  entity->SetMesh(terrain_mesh_id);
  entity->GetMaterial().model =
      glm::translate(glm::mat4{1.0f}, glm::vec3{0.0f, -0.06f, 0.0f});
  entity->GetMaterial().detail_scale_offset =
      glm::vec4{20.0f, 20.0f, 0.0f, 0.0f};

  Mesh plane_mesh;
  plane_mesh.LoadObjFile(FindAssetsFile("mesh/plane.obj"));
  auto plane_mesh_id = asset_manager->LoadMesh(plane_mesh, "PlaneMesh");

  Texture water_texture;
  water_texture.LoadFromFile(
      FindAssetsFile("texture/terrain/SkyBox/SkyBox5.bmp"));
  auto water_texture_id =
      asset_manager->LoadTexture(water_texture, "WaterTexture");
  int water_entity_id = scene_->CreateEntity();
  auto water_entity = scene_->GetEntity(water_entity_id);
  water_entity->SetAlbedoDetailTexture(water_texture_id);
  water_entity->SetMesh(plane_mesh_id);
  water_entity->GetMaterial().model =
      glm::scale(glm::mat4{1.0f}, glm::vec3{100.0f});
  water_entity->GetMaterial().detail_scale_offset =
      glm::vec4{1000.0f, 1000.0f, 0.0f, 0.0f};
  water_entity->GetMaterial().color = glm::vec4{1.5f, 1.5f, 1.5f, 0.5f};

  scene_->Camera()->GetFar() = 10.0f;
  scene_->Camera()->GetNear() = 0.01f;
  scene_->Camera()->GetPosition() = glm::vec3{0.0f, 0.1f, 1.2f};

  scene_->SetUpdateCallback([=](Scene *scene, float delta_time) {
    auto water_entity = scene->GetEntity(water_entity_id);
    glm::vec2 speed{0.3f, 1.0f};
    speed *= delta_time * 0.1f;
    water_entity->GetMaterial().detail_scale_offset.z += speed.x;
    water_entity->GetMaterial().detail_scale_offset.w += speed.y;
    water_entity->GetMaterial().detail_scale_offset.z =
        glm::mod(water_entity->GetMaterial().detail_scale_offset.z, 1.0f);
    water_entity->GetMaterial().detail_scale_offset.w =
        glm::mod(water_entity->GetMaterial().detail_scale_offset.w, 1.0f);
  });
}

}  // namespace sparks
