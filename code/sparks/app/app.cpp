#include "sparks/app/app.h"

#include <stb_image.h>
#include <stb_image_write.h>

#include "ImGuizmo.h"
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
  CreateGuiRenderer();
  RegisterInteractions();
  LoadScene();

  // Texture skybox[5];
  // skybox[0].LoadFromFile(FindAssetsFile("texture/terrain/SkyBox/SkyBox0.bmp"));
  // skybox[1].LoadFromFile(FindAssetsFile("texture/terrain/SkyBox/SkyBox1.bmp"));
  // skybox[2].LoadFromFile(FindAssetsFile("texture/terrain/SkyBox/SkyBox2.bmp"));
  // skybox[3].LoadFromFile(FindAssetsFile("texture/terrain/SkyBox/SkyBox3.bmp"));
  // skybox[4].LoadFromFile(FindAssetsFile("texture/terrain/SkyBox/SkyBox4.bmp"));
  // Texture envmap = SkyBoxToEnvmap(
  //     {&skybox[0], &skybox[1], &skybox[2], &skybox[3], &skybox[4]}, 2048);
  // envmap.SaveToFile("envmap.png");
}

void Application::OnClose() {
  DestroyGuiRenderer();
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

  CaptureMouseRelatedData();
  AppGuiInfo gui_info;
  gui_info.hovering_instance_id[0] = hovering_instances_[0];
  gui_info.hovering_instance_id[1] = hovering_instances_[1];
  gui_info.selected_instance_id[0] = selected_instances_[0];
  gui_info.selected_instance_id[1] = selected_instances_[1];
  gui_renderer_->UpdateGuiInfo(gui_info);

  scene_->Update(delta_time);
  render_settings_changed_ |= camera_controller_->Update(delta_time);

  imgui_manager_->BeginFrame();
  ImGuizmo::BeginFrame();
  ImGui();
  //  asset_manager_->ImGui();
  imgui_manager_->EndFrame();

  SceneSettings settings;
  scene_->GetSceneSettings(settings);
  if (render_settings_changed_) {
    if (settings.persistence == 1.0f) {
      reset_accumulated_buffer_ = true;
    }
    render_settings_changed_ = false;
  }

  if (reset_accumulated_buffer_) {
    raytracing_film_->ClearAccumulationBuffer();
    settings.accumulated_sample = 0;
    reset_accumulated_buffer_ = false;
  }
  scene_->SetSceneSettings(settings);

  asset_manager_->Update(core_->CurrentFrame());
  scene_->UpdatePipelineObjects();

  core_->TransferCommandPool()->SingleTimeCommands(
      core_->TransferQueue(), [&](VkCommandBuffer cmd_buffer) {
        scene_->SyncData(cmd_buffer, core_->CurrentFrame());
        gui_renderer_->SyncData(cmd_buffer, core_->CurrentFrame());
      });
}

void Application::OnRender() {
  core_->BeginFrame();
  VkCommandBuffer cmd_buffer = core_->CommandBuffer()->Handle();

  renderer_->RenderScene(cmd_buffer, film_.get(), scene_.get());

  //  vulkan::TransitImageLayout(
  //      cmd_buffer, frame_image_->Handle(), VK_IMAGE_LAYOUT_GENERAL,
  //      VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
  //      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
  //      VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
  //      VK_ACCESS_TRANSFER_READ_BIT, VK_IMAGE_ASPECT_COLOR_BIT);

  renderer_->RenderSceneRayTracing(cmd_buffer, raytracing_film_.get(),
                                   scene_.get());

  vulkan::TransitImageLayout(
      cmd_buffer, frame_image_->Handle(), VK_IMAGE_LAYOUT_UNDEFINED,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
      VK_PIPELINE_STAGE_TRANSFER_BIT, 0, VK_ACCESS_TRANSFER_WRITE_BIT,
      VK_IMAGE_ASPECT_COLOR_BIT);

  if (output_raytracing_result_) {
    vulkan::TransitImageLayout(
        cmd_buffer, raytracing_film_->result_image->Handle(),
        VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_SHADER_WRITE_BIT,
        VK_ACCESS_TRANSFER_READ_BIT, VK_IMAGE_ASPECT_COLOR_BIT);

    vulkan::BlitImage(cmd_buffer, raytracing_film_->result_image.get(),
                      frame_image_.get());

    vulkan::TransitImageLayout(
        cmd_buffer, raytracing_film_->result_image->Handle(),
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
        VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_SHADER_WRITE_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT);
  } else {
    vulkan::BlitImage(cmd_buffer, film_->result_image.get(),
                      frame_image_.get());
  }

  vulkan::TransitImageLayout(
      cmd_buffer, film_->stencil_image->Handle(),
      VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL,
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
      VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
      VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
      VK_IMAGE_ASPECT_COLOR_BIT);

  vulkan::TransitImageLayout(
      cmd_buffer, frame_image_->Handle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_TRANSFER_BIT,
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
      VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
      VK_IMAGE_ASPECT_COLOR_BIT);

  if (show_auxiliary_visual_components_) {
    gui_renderer_->Render(cmd_buffer);
  }

  vulkan::TransitImageLayout(
      cmd_buffer, film_->stencil_image->Handle(), VK_IMAGE_LAYOUT_GENERAL,
      VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
      VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
      VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_TRANSFER_READ_BIT,
      VK_IMAGE_ASPECT_COLOR_BIT);

  imgui_manager_->Render(cmd_buffer);

  vulkan::TransitImageLayout(
      cmd_buffer, frame_image_->Handle(), VK_IMAGE_LAYOUT_GENERAL,
      VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
      VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
      VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
      VK_IMAGE_ASPECT_COLOR_BIT);

  core_->OutputFrame(frame_image_.get());
  //  core_->OutputFrame(film_->intensity_image.get());
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
  if (gui_renderer_) {
    gui_renderer_->BindRelatedImages(frame_image_.get(),
                                     film_->stencil_image.get());
  }
}

void Application::DestroyFrameImage() {
  frame_image_.reset();
}

void Application::CreateImGuiManager() {
  imgui_manager_ = std::make_unique<vulkan::ImGuiManager>(
      core_.get(), frame_image_.get(),
      FindAssetsFile("fonts/NotoSans-Regular.ttf").c_str(), 20.0f);
  ImGuizmo::Enable(true);
}

void Application::DestroyImGuiManager() {
  imgui_manager_.reset();
}

void Application::CreateAssetManager() {
  asset_manager_ = std::make_unique<AssetManager>(core_.get(), 8192, 8192);
}

void Application::DestroyAssetManager() {
  asset_manager_.reset();
}

void Application::CreateRenderer() {
  renderer_ = std::make_unique<Renderer>(asset_manager_.get());
  renderer_->CreateFilm(core_->Swapchain()->Extent().width,
                        core_->Swapchain()->Extent().height, &film_);
  renderer_->CreateRayTracingFilm(core_->Swapchain()->Extent().width,
                                  core_->Swapchain()->Extent().height,
                                  &raytracing_film_);
  core_->FrameSizeEvent().RegisterCallback([this](int width, int height) {
    film_->Resize(width, height);
    raytracing_film_->Resize(width, height);
    reset_accumulated_buffer_ = true;

    gui_renderer_->BindRelatedImages(frame_image_.get(),
                                     film_->stencil_image.get());
  });
  renderer_->CreateScene(2, &scene_);

  camera_controller_ =
      std::make_unique<CameraController>(core_.get(), scene_->Camera());
}

void Application::DestroyRenderer() {
  camera_controller_.reset();
  scene_.reset();
  raytracing_film_.reset();
  film_.reset();
  renderer_.reset();
}

void Application::CreateGuiRenderer() {
  gui_renderer_ = std::make_unique<AppGuiRenderer>(this);

  gui_renderer_->BindRelatedImages(frame_image_.get(),
                                   film_->stencil_image.get());
}

void Application::DestroyGuiRenderer() {
  gui_renderer_.reset();
}

void Application::LoadScene() {
  auto asset_manager = scene_->Renderer()->AssetManager();
  scene_->Camera()->GetPosition() = glm::vec3{0.0f, 0.0f, 5.0f};

  auto envmap = scene_->GetEnvMap();

  Texture envmap_texture;
  envmap_texture.LoadFromFile(FindAssetsFile("texture/envmap_clouds_4k.hdr"),
                              LDRColorSpace::UNORM);
  auto envmap_id = asset_manager->LoadTexture(envmap_texture, "Envmap");
  envmap->SetEnvmapTexture(envmap_id);
  scene_->SetEnvmapSettings({0.0f, 1.0f, uint32_t(envmap_id), 0});

  int entity_id = scene_->CreateEntity();

  Texture terrain_texture;
  terrain_texture.LoadFromFile(
      FindAssetsFile("texture/terrain/terrain-texture3.bmp"),
      LDRColorSpace::UNORM);

  Texture terrain_detail_texture;
  terrain_detail_texture.LoadFromFile(
      FindAssetsFile("texture/terrain/detail.bmp"), LDRColorSpace::UNORM);
  auto terrain_texture_id =
      asset_manager->LoadTexture(terrain_texture, "TerrainTexture");
  auto terrain_detail_texture_id = asset_manager->LoadTexture(
      terrain_detail_texture, "TerrainDetailTexture");

  scene_->SetEntityAlbedoTexture(entity_id, terrain_texture_id);
  scene_->SetEntityAlbedoDetailTexture(entity_id, terrain_detail_texture_id);

  Texture heightmap_texture;
  heightmap_texture.LoadFromFile(
      FindAssetsFile("texture/terrain/heightmap.bmp"), LDRColorSpace::UNORM);
  Mesh terrain_mesh;
  terrain_mesh.LoadFromHeightMap(heightmap_texture, 1.0f, 0.2f, 0.0f);
  auto terrain_mesh_id = asset_manager->LoadMesh(terrain_mesh, "TerrainMesh");
  Material terrain_material;
  terrain_material.sheen = 1.0f;
  scene_->SetEntityMesh(entity_id, terrain_mesh_id);
  scene_->SetEntityMaterial(entity_id, terrain_material);
  scene_->SetEntityTransform(
      entity_id,
      glm::translate(glm::mat4{1.0f}, glm::vec3{0.0f, -0.06f, 0.0f}));
  scene_->SetEntityDetailScaleOffset(entity_id, {20.0f, 20.0f, 0.0f, 0.0f});

  Mesh plane_mesh;
  std::vector<Vertex> plane_vertices;
  std::vector<uint32_t> plane_indices;
  const int precision = 500;
  const float inv_precision = 1.0f / static_cast<float>(precision);
  for (int i = 0; i <= precision; i++) {
    for (int j = 0; j <= precision; j++) {
      Vertex vertex;
      vertex.position = {static_cast<float>(i) * inv_precision - 0.5f, 0.0f,
                         static_cast<float>(j) * inv_precision - 0.5f};
      vertex.normal = {0.0f, 1.0f, 0.0f};
      vertex.tangent = {1.0f, 0.0f, 0.0f};
      vertex.tex_coord = {static_cast<float>(i) * inv_precision,
                          1.0f - static_cast<float>(j) * inv_precision};
      vertex.signal = 1.0f;
      plane_vertices.push_back(vertex);
    }
  }
  for (int i = 0; i < precision; i++) {
    for (int j = 0; j < precision; j++) {
      plane_indices.push_back(i * (precision + 1) + j);
      plane_indices.push_back(i * (precision + 1) + j + 1);
      plane_indices.push_back((i + 1) * (precision + 1) + j);
      plane_indices.push_back((i + 1) * (precision + 1) + j);
      plane_indices.push_back(i * (precision + 1) + j + 1);
      plane_indices.push_back((i + 1) * (precision + 1) + j + 1);
    }
  }
  plane_mesh = {plane_vertices, plane_indices};

  auto plane_mesh_id = asset_manager->LoadMesh(plane_mesh, "PlaneMesh");

  Texture water_texture;
  water_texture.LoadFromFile(
      FindAssetsFile("texture/terrain/SkyBox/SkyBox5.bmp"),
      LDRColorSpace::UNORM);
  auto water_texture_id =
      asset_manager->LoadTexture(water_texture, "WaterTexture");
  int water_entity_id = scene_->CreateEntity();
  Material water_material;
  water_material.base_color = {1.0f, 1.0f, 1.0f};
  water_material.metallic = 0.1f;
  water_material.specular = 1.0f;
  water_material.alpha = 1.0f;
  scene_->SetEntityAlbedoDetailTexture(water_entity_id, water_texture_id);
  scene_->SetEntityMesh(water_entity_id, plane_mesh_id);
  scene_->SetEntityMaterial(water_entity_id, water_material);
  scene_->SetEntityTransform(water_entity_id,
                             glm::scale(glm::mat4{1.0f}, glm::vec3{1000.0f}));
  scene_->SetEntityDetailScaleOffset(water_entity_id,
                                     {10000.0f, 10000.0f, 0.0f, 0.0f});

  scene_->Camera()->SetFar(500.0f);
  scene_->Camera()->SetNear(0.05f);
  scene_->Camera()->SetPosition({0.0f, 0.1f, 1.2f});

  scene_->SetUpdateCallback([=](Scene *scene, float delta_time) {
    glm::vec2 speed{0.3f, 1.0f};
    speed *= delta_time * 0.1f;
    glm::vec4 detail_scale_offset;
    scene->GetEntityDetailScaleOffset(water_entity_id, detail_scale_offset);
    detail_scale_offset.z += speed.x;
    detail_scale_offset.w += speed.y;
    detail_scale_offset.z = glm::mod(detail_scale_offset.z, 1.0f);
    detail_scale_offset.w = glm::mod(detail_scale_offset.w, 1.0f);
    scene->SetEntityDetailScaleOffset(water_entity_id, detail_scale_offset);
  });
}

void Application::ImGui() {
  if (show_auxiliary_visual_components_) {
    ImGuiSettingsWindow();
    ImVec2 window_size = ImGuizmoWindow();
    window_size = ImGuiStatisticWindow(
        ImVec2(ImGui::GetIO().DisplaySize.x, window_size.y));
  }
}

void Application::CaptureMouseRelatedData() {
  double x_pos, y_pos;
  auto window = core_->Swapchain()->Surface()->Window();
  glfwGetCursorPos(window, &x_pos, &y_pos);
  glfwGetWindowSize(window, &window_width_, &window_height_);
  glfwGetFramebufferSize(window, &framebuffer_width_, &framebuffer_height_);
  x_pos *= static_cast<double>(framebuffer_width_) /
           static_cast<double>(window_width_);
  y_pos *= static_cast<double>(framebuffer_height_) /
           static_cast<double>(window_height_);

  int x_focus = std::lround(x_pos), y_focus = std::lround(y_pos);
  VkExtent2D extent = core_->Swapchain()->Extent();

  hovering_instances_[0] = 0xfffffffeu;
  hovering_instances_[1] = 0xfffffffeu;
  hovering_color_ = glm::vec4{0.0f};
  if (x_focus >= 0 && x_focus < extent.width && y_focus >= 0 &&
      y_focus < extent.height) {
    film_->stencil_image->FetchPixelData(
        core_->GraphicsCommandPool(), core_->GraphicsQueue(),
        VkRect2D{{x_focus, y_focus}, {1, 1}}, hovering_instances_,
        sizeof(hovering_instances_));
    raytracing_film_->raw_result_image->FetchPixelData(
        core_->GraphicsCommandPool(), core_->GraphicsQueue(),
        VkRect2D{{x_focus, y_focus}, {1, 1}}, &hovering_color_,
        sizeof(hovering_color_), VK_IMAGE_LAYOUT_GENERAL);
  }

  cursor_x_ = x_focus;
  cursor_y_ = y_focus;
}

void Application::RegisterInteractions() {
  core_->MouseButtonEvent().RegisterCallback(
      [this](int button, int action, int mods) {
        auto &io = ImGui::GetIO();
        if (io.WantCaptureMouse) {
          return;
        }
        if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
          if (hovering_instances_[0] != 0xffffffffu) {
            pre_selected_instances[0] = hovering_instances_[0];
            pre_selected_instances[1] = hovering_instances_[1];
          }
        } else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
          if (hovering_instances_[0] == pre_selected_instances[0] &&
              hovering_instances_[1] == pre_selected_instances[1]) {
            selected_instances_[0] = pre_selected_instances[0];
            selected_instances_[1] = pre_selected_instances[1];
          }
        } else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
          selected_instances_[0] = 0xfffffffeu;
          selected_instances_[1] = 0xfffffffeu;
        }
      });
  core_->CursorPosEvent().RegisterCallback([this](double x, double y) {
    auto &io = ImGui::GetIO();
    if (io.WantCaptureMouse) {
      return;
    }
    pre_selected_instances[0] = 0xfffffffeu;
    pre_selected_instances[1] = 0xfffffffeu;
  });
  core_->KeyEvent().RegisterCallback([this](int key, int scancode, int action,
                                            int mods) {
    if (key == GLFW_KEY_G && action == GLFW_PRESS) {
      show_auxiliary_visual_components_ = !show_auxiliary_visual_components_;
    }
  });
}

ImVec2 Application::ImGuiSettingsWindow() {
  ImVec2 window_size = ImVec2{};
  ImGui::SetNextWindowPos({0, 0}, ImGuiCond_Once);
  ImGui::SetNextWindowBgAlpha(0.3);
  ImGui::Begin("Global Settings", nullptr,
               ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize |
                   ImGuiWindowFlags_NoResize | ImGuiWindowFlags_MenuBar);
  window_size = ImGui::GetWindowSize();
  if (ImGui::BeginMenuBar()) {
    if (ImGui::BeginMenu("File")) {
      char *result{nullptr};
      if (ImGui::MenuItem("Open Scene")) {
      }
      if (ImGui::MenuItem("Import Image..")) {
      }
      if (ImGui::MenuItem("Import Mesh..")) {
      }
      ImGui::Separator();
      if (ImGui::MenuItem("Capture and Save..")) {
      }
      ImGui::EndMenu();
    }
    ImGui::EndMenuBar();
  }

  if (ImGui::RadioButton("Preview", !output_raytracing_result_)) {
    output_raytracing_result_ = false;
  }
  ImGui::SameLine();
  if (ImGui::RadioButton("Ray Tracing", output_raytracing_result_)) {
    output_raytracing_result_ = true;
  }

  ImGui::SeparatorText("Note");
  ImGui::Text("W/A/S/D/LCTRL/SPACE for camera movement.");
  ImGui::Text("Cursor drag on frame for camera rotation.");
  //        ImGui::Text("G for show/hidse GUI windows.");

  if (ImGui::CollapsingHeader("Scene Settings")) {
    scene_->GetSceneSettings(editing_scene_settings_);
    render_settings_changed_ |= ImGui::SliderInt(
        "Bounces",
        reinterpret_cast<int *>(&editing_scene_settings_.num_bounces), 1, 128);
    render_settings_changed_ |= ImGui::SliderInt(
        "Samples", reinterpret_cast<int *>(&editing_scene_settings_.num_sample),
        1, 128);
    render_settings_changed_ |= ImGui::SliderFloat(
        "Exposure", &editing_scene_settings_.exposure, 0.0f, 2.0f, "%.2f");
    render_settings_changed_ |= ImGui::SliderFloat(
        "Gamma", &editing_scene_settings_.gamma, 0.1f, 5.0f, "%.2f");
    render_settings_changed_ |=
        ImGui::SliderFloat("Persistence", &editing_scene_settings_.persistence,
                           0.0f, 1.0f, "%.2f");
    render_settings_changed_ |=
        ImGui::SliderFloat("Clamp", &editing_scene_settings_.clamp_value, 1.0f,
                           10000.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
    scene_->SetSceneSettings(editing_scene_settings_);
  }
  if (ImGui::CollapsingHeader("Environment Map Settings")) {
    scene_->GetEnvmapSettings(editing_envmap_settings_);
    render_settings_changed_ |= asset_manager_->ComboForTextureSelection(
        "Envmap", &editing_envmap_settings_.envmap_id);
    render_settings_changed_ |= ImGui::SliderFloat(
        "Offset", &editing_envmap_settings_.offset, 0.0f, 1.0f);
    render_settings_changed_ |= ImGui::SliderFloat(
        "Scale", &editing_envmap_settings_.scale, 0.0f, 2.0f, "%.2f");
    bool reflect = editing_envmap_settings_.reflect;
    ImGui::Checkbox("Reflect", &reflect);
    editing_envmap_settings_.reflect = reflect;
    scene_->SetEnvmapSettings(editing_envmap_settings_);
  }
  auto entity = scene_->GetEntity(selected_instances_[0]);
  if (entity) {
    if (ImGui::CollapsingHeader("Material")) {
      Material material;
      EntityMetadata metadata;
      scene_->GetEntityMaterial(selected_instances_[0], material);
      scene_->GetEntityMetadata(selected_instances_[0], metadata);
      render_settings_changed_ |=
          asset_manager_->ComboForMeshSelection("Mesh", &metadata.mesh_id);

      const char *material_type_names[] = {"Lambertian", "Specular",
                                           "Principled"};
      render_settings_changed_ |=
          ImGui::Combo("Material Type", reinterpret_cast<int *>(&material.type),
                       material_type_names, 3);

      render_settings_changed_ |=
          ImGui::ColorEdit3("Base Color", &material.base_color.r);
      render_settings_changed_ |= asset_manager_->ComboForTextureSelection(
          "Base Color Texture", &metadata.albedo_texture_id);
      render_settings_changed_ |= asset_manager_->ComboForTextureSelection(
          "Detail Texture", &metadata.albedo_detail_texture_id);
      render_settings_changed_ |= ImGui::SliderFloat2(
          "Detail Scale", &metadata.detail_scale_offset.x, 0.0001f, 10000.0f,
          "%.4f", ImGuiSliderFlags_Logarithmic);
      render_settings_changed_ |= ImGui::SliderFloat2(
          "Detail Offset", &metadata.detail_scale_offset.z, 0.0f, 1.0f, "%.4f");

      if (material.type == 2) {
        render_settings_changed_ |=
            ImGui::SliderFloat("Metallic", &material.metallic, 0.0f, 1.0f);
        render_settings_changed_ |=
            ImGui::SliderFloat("Roughness", &material.roughness, 0.0f, 1.0f);
        render_settings_changed_ |=
            ImGui::SliderFloat("Specular", &material.specular, 0.0f, 1.0f);
        render_settings_changed_ |= ImGui::SliderFloat(
            "Specular Tint", &material.specular_tint, 0.0f, 1.0f);
        render_settings_changed_ |=
            ImGui::SliderFloat("Subsurface", &material.subsurface, 0.0f, 1.0f);
        render_settings_changed_ |= ImGui::SliderFloat3(
            "Subsurface Radius", &material.subsurface_radius.x, 0.0f, 10.0f);
        render_settings_changed_ |=
            ImGui::ColorEdit3("Subsurface Color", &material.subsurface_color.r);
        render_settings_changed_ |= ImGui::SliderFloat(
            "Anisotropic", &material.anisotropic, 0.0f, 1.0f);
        render_settings_changed_ |= ImGui::SliderFloat(
            "Anisotropic Rotation", &material.anisotropic_rotation, 0.0f, 1.0f);
        render_settings_changed_ |=
            ImGui::SliderFloat("Sheen", &material.sheen, 0.0f, 1.0f);
        render_settings_changed_ |=
            ImGui::SliderFloat("Sheen Tint", &material.sheen_tint, 0.0f, 1.0f);
        render_settings_changed_ |=
            ImGui::SliderFloat("Clearcoat", &material.clearcoat, 0.0f, 1.0f);
        render_settings_changed_ |= ImGui::SliderFloat(
            "Clearcoat Roughness", &material.clearcoat_roughness, 0.0f, 1.0f);
        render_settings_changed_ |=
            ImGui::SliderFloat("IOR", &material.ior, 0.1f, 10.0f, "%.2f",
                               ImGuiSliderFlags_Logarithmic);
        render_settings_changed_ |= ImGui::SliderFloat(
            "Transmission", &material.transmission, 0.0f, 1.0f);
        render_settings_changed_ |=
            ImGui::SliderFloat("Transmission Roughness",
                               &material.transmission_roughness, 0.0f, 1.0f);
      }

      render_settings_changed_ |=
          ImGui::ColorEdit3("Emission", &material.emission.r);
      render_settings_changed_ |= ImGui::SliderFloat(
          "Emission Strength", &material.emission_strength, 0.0f, 10.0f);

      render_settings_changed_ |=
          ImGui::SliderFloat("Alpha", &material.alpha, 0.0f, 1.0f);

      scene_->SetEntityMaterial(selected_instances_[0], material);
      scene_->SetEntityMetadata(selected_instances_[0], metadata);
    }
  }
  ImGui::End();
  return window_size;
}

ImVec2 Application::ImGuizmoWindow() {
  ImVec2 window_size{0.0f, 0.0f};
  if ((selected_instances_[0] & 0xffffff00u) != 0xffffff00u) {
    auto entity = scene_->GetEntity(selected_instances_[0]);
    if (entity) {
      editing_transform_ = entity->GetTranslatedMetadata().transform;
      ImGui::SetNextWindowPos(
          ImVec2(ImGui::GetIO().DisplaySize.x, window_size.y), ImGuiCond_Always,
          ImVec2(1.0f, 0.0f));
      ImGui::SetNextWindowBgAlpha(0.3);
      ImGui::Begin("Gizmo", nullptr,
                   ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove |
                       ImGuiWindowFlags_NoTitleBar);
      window_size = ImGui::GetWindowSize();
      ImGui::Text("Gizmo");
      ImGui::Separator();
      bool value_changed = false;
      static ImGuizmo::OPERATION current_guizmo_operation(ImGuizmo::ROTATE);
      static ImGuizmo::MODE current_guizmo_mode(ImGuizmo::WORLD);
      if (ImGui::IsKeyPressed(ImGuiKey_T)) {
        current_guizmo_operation = ImGuizmo::TRANSLATE;
      }
      if (ImGui::IsKeyPressed(ImGuiKey_R)) {
        current_guizmo_operation = ImGuizmo::ROTATE;
      }
      if (ImGui::IsKeyPressed(ImGuiKey_S)) {
        current_guizmo_operation = ImGuizmo::SCALE;
      }
      float matrixTranslation[3], matrixRotation[3], matrixScale[3];
      auto &io = ImGui::GetIO();

      if (ImGui::RadioButton("Translate",
                             current_guizmo_operation == ImGuizmo::TRANSLATE)) {
        current_guizmo_operation = ImGuizmo::TRANSLATE;
      }
      ImGui::SameLine();
      if (ImGui::RadioButton("Rotate",
                             current_guizmo_operation == ImGuizmo::ROTATE)) {
        current_guizmo_operation = ImGuizmo::ROTATE;
      }
      ImGui::SameLine();
      if (ImGui::RadioButton("Scale",
                             current_guizmo_operation == ImGuizmo::SCALE)) {
        current_guizmo_operation = ImGuizmo::SCALE;
      }
      ImGuizmo::DecomposeMatrixToComponents(
          reinterpret_cast<float *>(&editing_transform_), matrixTranslation,
          matrixRotation, matrixScale);
      value_changed |= ImGui::InputFloat3("Translation", matrixTranslation);
      value_changed |= ImGui::InputFloat3("Rotation", matrixRotation);
      value_changed |= ImGui::InputFloat3("Scale", matrixScale);
      ImGuizmo::RecomposeMatrixFromComponents(
          matrixTranslation, matrixRotation, matrixScale,
          reinterpret_cast<float *>(&editing_transform_));

      if (current_guizmo_operation != ImGuizmo::SCALE) {
        if (ImGui::RadioButton("Local",
                               current_guizmo_mode == ImGuizmo::LOCAL)) {
          current_guizmo_mode = ImGuizmo::LOCAL;
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("World",
                               current_guizmo_mode == ImGuizmo::WORLD)) {
          current_guizmo_mode = ImGuizmo::WORLD;
        }
      }

      glm::mat4 imguizmo_view_ = scene_->Camera()->GetView();
      glm::mat4 imguizmo_proj_ =
          glm::scale(glm::mat4{1.0f}, glm::vec3{1.0f, 1.0f, 1.0f}) *
          scene_->Camera()->GetProjection(float(io.DisplaySize.x) /
                                          float(io.DisplaySize.y));
      ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);
      value_changed |= ImGuizmo::Manipulate(
          reinterpret_cast<float *>(&imguizmo_view_),
          reinterpret_cast<float *>(&imguizmo_proj_), current_guizmo_operation,
          current_guizmo_mode, reinterpret_cast<float *>(&editing_transform_),
          nullptr, nullptr);
      ImGui::End();
      scene_->SetEntityTransform(selected_instances_[0], editing_transform_);
    }
  }
  return window_size;
}

ImVec2 Application::ImGuiStatisticWindow(ImVec2 window_pos) {
  ImVec2 window_size{0.0f, 0.0f};
  ImGui::SetNextWindowPos(ImVec2(window_pos), ImGuiCond_Always,
                          ImVec2(1.0f, 0.0f));
  ImGui::SetNextWindowBgAlpha(0.3f);
  ImGui::Begin("Statistics", nullptr,
               ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize |
                   ImGuiWindowFlags_NoTitleBar);
  ImGui::Text("Statistics (%dx%d)", framebuffer_width_, framebuffer_height_);
  ImGui::Text("Cursor: (%d, %d)", cursor_x_, cursor_y_);
  if (cursor_x_ >= 0 && cursor_x_ < framebuffer_width_ && cursor_y_ >= 0 &&
      cursor_y_ < framebuffer_height_) {
    ImGui::Text("Hovering: %x %x", hovering_instances_[0],
                hovering_instances_[1]);
    ImGui::Text("Hovering Color: (%.2f, %.2f, %.2f)", hovering_color_.r,
                hovering_color_.g, hovering_color_.b);
  }
  SceneSettings scene_settings;
  scene_->GetSceneSettings(scene_settings);
  ImGui::Text("Accumulated Sample: %u", scene_settings.accumulated_sample);
  scene_->SetSceneSettings(scene_settings);
  window_size = ImGui::GetWindowSize();
  ImGui::End();
  return window_size;
}

}  // namespace sparks
