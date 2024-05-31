#include "sparks/renderer/renderer.h"

#include "sparks/scene/scene.h"

namespace sparks {

namespace {

#include "built_in_shaders.inl"

}

Renderer::Renderer(class AssetManager *asset_manager)
    : core_(asset_manager->Core()), asset_manager_(asset_manager) {
  CreateDescriptorSetLayouts();
  CreateRenderPass();
  CreateEnvmapPipeline();
  CreateEntityPipeline();
  CreateRayTracingPipeline();
}

Renderer::~Renderer() {
  DestroyRayTracingPipeline();
  DestroyEntityPipeline();
  DestroyEnvmapPipeline();
  DestroyRenderPass();
  DestroyDescriptorSetLayouts();
}

void Renderer::CreateDescriptorSetLayouts() {
  core_->Device()->CreateSampler(
      VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT,
      VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_FALSE,
      VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE, VK_SAMPLER_MIPMAP_MODE_LINEAR,
      &sampler_);

  VkSampler sampler = sampler_->Handle();

  core_->Device()->CreateDescriptorSetLayout(
      {{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1,
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_RAYGEN_BIT_KHR, nullptr},
       {1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_RAYGEN_BIT_KHR,
        nullptr},
       {2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_RAYGEN_BIT_KHR,
        nullptr}},
      &scene_descriptor_set_layout_);
  core_->Device()->CreateDescriptorSetLayout(
      {{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1,
        VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_RAYGEN_BIT_KHR, nullptr},
       {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
        VK_SHADER_STAGE_FRAGMENT_BIT, &sampler}},
      &envmap_descriptor_set_layout_);
  core_->Device()->CreateDescriptorSetLayout(
      {{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1,
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
       {1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1,
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
       {2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
        VK_SHADER_STAGE_FRAGMENT_BIT, &sampler},
       {3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
        VK_SHADER_STAGE_FRAGMENT_BIT, &sampler}},
      &entity_descriptor_set_layout_);
}

void Renderer::DestroyDescriptorSetLayouts() {
  entity_descriptor_set_layout_.reset();
  envmap_descriptor_set_layout_.reset();
  scene_descriptor_set_layout_.reset();
  sampler_.reset();
}

void Renderer::CreateRenderPass() {
  std::vector<VkAttachmentDescription> descriptions;
  std::vector<vulkan::SubpassSettings> subpasses;
  std::vector<VkSubpassDependency> dependencies;
  descriptions.emplace_back(VkAttachmentDescription{
      0, kAlbedoFormat, VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR,
      VK_ATTACHMENT_STORE_OP_STORE, VK_ATTACHMENT_LOAD_OP_DONT_CARE,
      VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_UNDEFINED,
      VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL});
  descriptions.emplace_back(VkAttachmentDescription{
      0, kPositionFormat, VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR,
      VK_ATTACHMENT_STORE_OP_STORE, VK_ATTACHMENT_LOAD_OP_DONT_CARE,
      VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_UNDEFINED,
      VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL});
  descriptions.emplace_back(VkAttachmentDescription{
      0, kNormalFormat, VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR,
      VK_ATTACHMENT_STORE_OP_STORE, VK_ATTACHMENT_LOAD_OP_DONT_CARE,
      VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_UNDEFINED,
      VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL});
  descriptions.emplace_back(VkAttachmentDescription{
      0, kRadianceFormat, VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR,
      VK_ATTACHMENT_STORE_OP_STORE, VK_ATTACHMENT_LOAD_OP_DONT_CARE,
      VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_UNDEFINED,
      VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL});
  descriptions.emplace_back(VkAttachmentDescription{
      0, kDepthFormat, VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR,
      VK_ATTACHMENT_STORE_OP_STORE, VK_ATTACHMENT_LOAD_OP_DONT_CARE,
      VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_UNDEFINED,
      VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL});
  descriptions.emplace_back(VkAttachmentDescription{
      0, kStencilFormat, VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR,
      VK_ATTACHMENT_STORE_OP_STORE, VK_ATTACHMENT_LOAD_OP_DONT_CARE,
      VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_UNDEFINED,
      VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL});

  vulkan::SubpassSettings subpass;
  subpass.color_attachment_references.push_back(
      VkAttachmentReference{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});
  subpass.color_attachment_references.push_back(
      VkAttachmentReference{1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});
  subpass.color_attachment_references.push_back(
      VkAttachmentReference{2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});
  subpass.color_attachment_references.push_back(
      VkAttachmentReference{3, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});
  subpass.color_attachment_references.push_back(
      VkAttachmentReference{5, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});
  subpass.depth_attachment_reference = VkAttachmentReference{
      4, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
  subpasses.push_back(subpass);

  dependencies.emplace_back(VkSubpassDependency{
      VK_SUBPASS_EXTERNAL, 0, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_MEMORY_READ_BIT,
      VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
          VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
      0});

  core_->Device()->CreateRenderPass(descriptions, subpasses, dependencies,
                                    &render_pass_);
}

void Renderer::DestroyRenderPass() {
  render_pass_.reset();
}

void Renderer::CreateEnvmapPipeline() {
  core_->Device()->CreatePipelineLayout(
      {scene_descriptor_set_layout_->Handle(),
       envmap_descriptor_set_layout_->Handle()},
      &envmap_pipeline_layout_);

  core_->Device()->CreateShaderModule(
      vulkan::CompileGLSLToSPIRV(GetShaderCode("shaders/envmap_pass.vert"),
                                 VK_SHADER_STAGE_VERTEX_BIT),
      &envmap_vertex_shader_);
  core_->Device()->CreateShaderModule(
      vulkan::CompileGLSLToSPIRV(GetShaderCode("shaders/envmap_pass.frag"),
                                 VK_SHADER_STAGE_FRAGMENT_BIT),
      &envmap_fragment_shader_);

  vulkan::PipelineSettings pipeline_settings{render_pass_.get(),
                                             envmap_pipeline_layout_.get(), 0};
  pipeline_settings.depth_stencil_state_create_info->depthTestEnable = false;
  pipeline_settings.depth_stencil_state_create_info->depthWriteEnable = false;
  pipeline_settings.AddShaderStage(envmap_vertex_shader_.get(),
                                   VK_SHADER_STAGE_VERTEX_BIT);
  pipeline_settings.AddShaderStage(envmap_fragment_shader_.get(),
                                   VK_SHADER_STAGE_FRAGMENT_BIT);
  core_->Device()->CreatePipeline(pipeline_settings, &envmap_pipeline_);
}

void Renderer::DestroyEnvmapPipeline() {
  envmap_pipeline_.reset();

  envmap_vertex_shader_.reset();
  envmap_fragment_shader_.reset();

  envmap_pipeline_layout_.reset();
}

void Renderer::CreateEntityPipeline() {
  core_->Device()->CreatePipelineLayout(
      {scene_descriptor_set_layout_->Handle(),
       entity_descriptor_set_layout_->Handle()},
      &entity_pipeline_layout_);

  core_->Device()->CreateShaderModule(
      vulkan::CompileGLSLToSPIRV(GetShaderCode("shaders/entity_pass.vert"),
                                 VK_SHADER_STAGE_VERTEX_BIT),
      &entity_vertex_shader_);
  core_->Device()->CreateShaderModule(
      vulkan::CompileGLSLToSPIRV(GetShaderCode("shaders/entity_pass.frag"),
                                 VK_SHADER_STAGE_FRAGMENT_BIT),
      &entity_fragment_shader_);

  vulkan::PipelineSettings pipeline_settings{render_pass_.get(),
                                             entity_pipeline_layout_.get(), 0};
  pipeline_settings.AddInputBinding(0, sizeof(Vertex),
                                    VK_VERTEX_INPUT_RATE_VERTEX);
  pipeline_settings.AddInputAttribute(0, 0, VK_FORMAT_R32G32B32_SFLOAT,
                                      offsetof(Vertex, position));
  pipeline_settings.AddInputAttribute(0, 1, VK_FORMAT_R32G32B32_SFLOAT,
                                      offsetof(Vertex, normal));
  pipeline_settings.AddInputAttribute(0, 2, VK_FORMAT_R32G32B32_SFLOAT,
                                      offsetof(Vertex, tangent));
  pipeline_settings.AddInputAttribute(0, 3, VK_FORMAT_R32G32_SFLOAT,
                                      offsetof(Vertex, tex_coord));
  pipeline_settings.AddInputAttribute(0, 4, VK_FORMAT_R32_SFLOAT,
                                      offsetof(Vertex, signal));

  pipeline_settings.AddShaderStage(entity_vertex_shader_.get(),
                                   VK_SHADER_STAGE_VERTEX_BIT);
  pipeline_settings.AddShaderStage(entity_fragment_shader_.get(),
                                   VK_SHADER_STAGE_FRAGMENT_BIT);

  pipeline_settings.SetBlendState(
      3, VkPipelineColorBlendAttachmentState{
             VK_TRUE, VK_BLEND_FACTOR_SRC_ALPHA,
             VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, VK_BLEND_OP_ADD,
             VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO, VK_BLEND_OP_ADD,
             VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                 VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT

         });

  pipeline_settings.SetPrimitiveTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
  pipeline_settings.SetCullMode(VK_CULL_MODE_NONE);

  core_->Device()->CreatePipeline(pipeline_settings, &entity_pipeline_);
}

void Renderer::DestroyEntityPipeline() {
  entity_pipeline_.reset();

  entity_vertex_shader_.reset();
  entity_fragment_shader_.reset();

  entity_pipeline_layout_.reset();
}

void Renderer::CreateRayTracingPipeline() {
  core_->Device()->CreateDescriptorSetLayout(
      {{0, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1,
        VK_SHADER_STAGE_RAYGEN_BIT_KHR, nullptr}},
      &raytracing_descriptor_set_layout_);

  core_->Device()->CreateDescriptorSetLayout(
      {{0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_RAYGEN_BIT_KHR,
        nullptr}},
      &raytracing_film_descriptor_set_layout_);

  core_->Device()->CreatePipelineLayout(
      {scene_descriptor_set_layout_->Handle(),
       raytracing_descriptor_set_layout_->Handle(),
       asset_manager_->DescriptorSetLayout()->Handle(),
       envmap_descriptor_set_layout_->Handle(),
       raytracing_film_descriptor_set_layout_->Handle()},
      &raytracing_pipeline_layout_);

  core_->Device()->CreateShaderModule(
      vulkan::CompileGLSLToSPIRV(GetShaderCode("shaders/raytracing.rgen"),
                                 VK_SHADER_STAGE_RAYGEN_BIT_KHR),
      &raytracing_raygen_shader_);

  core_->Device()->CreateShaderModule(
      vulkan::CompileGLSLToSPIRV(GetShaderCode("shaders/raytracing.rmiss"),
                                 VK_SHADER_STAGE_MISS_BIT_KHR),
      &raytracing_miss_shader_);

  core_->Device()->CreateShaderModule(
      vulkan::CompileGLSLToSPIRV(GetShaderCode("shaders/raytracing.rchit"),
                                 VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR),
      &raytracing_closest_hit_shader_);

  core_->Device()->CreateRayTracingPipeline(
      raytracing_pipeline_layout_.get(), raytracing_raygen_shader_.get(),
      raytracing_miss_shader_.get(), raytracing_closest_hit_shader_.get(),
      &raytracing_pipeline_);

  core_->Device()->CreateShaderBindingTable(raytracing_pipeline_.get(),
                                            &raytracing_sbt_);
}

void Renderer::DestroyRayTracingPipeline() {
  raytracing_sbt_.reset();
  raytracing_pipeline_.reset();
  raytracing_closest_hit_shader_.reset();
  raytracing_miss_shader_.reset();
  raytracing_raygen_shader_.reset();
  raytracing_pipeline_layout_.reset();
  raytracing_film_descriptor_set_layout_.reset();
  raytracing_descriptor_set_layout_.reset();
}

int Renderer::CreateScene(int max_entities, double_ptr<class Scene> pp_scene) {
  pp_scene.construct(this, max_entities);
  return 0;
}

int Renderer::CreateFilm(uint32_t width,
                         uint32_t height,
                         double_ptr<Film> pp_film) {
  Film film;
  film.renderer = this;
  Core()->Device()->CreateImage(
      kAlbedoFormat, VkExtent2D{width, height},
      VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
      &film.albedo_image);
  Core()->Device()->CreateImage(
      kPositionFormat, VkExtent2D{width, height},
      VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
      &film.position_image);
  Core()->Device()->CreateImage(
      kNormalFormat, VkExtent2D{width, height},
      VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
      &film.normal_image);
  Core()->Device()->CreateImage(
      kRadianceFormat, VkExtent2D{width, height},
      VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
      &film.radiance_image);
  Core()->Device()->CreateImage(kDepthFormat, VkExtent2D{width, height},
                                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
                                    VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                                &film.depth_image);
  Core()->Device()->CreateImage(
      kStencilFormat, VkExtent2D{width, height},
      VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
      &film.stencil_image);

  Core()->SingleTimeCommands([&](VkCommandBuffer cmd_buffer) {
    vulkan::TransitImageLayout(
        cmd_buffer, film.stencil_image->Handle(), VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT, 0, VK_ACCESS_TRANSFER_READ_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT);
  });

  RenderPass()->CreateFramebuffer(
      {film.albedo_image->ImageView(), film.position_image->ImageView(),
       film.normal_image->ImageView(), film.radiance_image->ImageView(),
       film.depth_image->ImageView(), film.stencil_image->ImageView()},
      VkExtent2D{width, height}, &film.framebuffer);
  pp_film.construct(std::move(film));
  return 0;
}

int Renderer::CreateRayTracingFilm(uint32_t width,
                                   uint32_t height,
                                   double_ptr<RayTracingFilm> pp_film) {
  RayTracingFilm film;

  film.renderer = this;

  Core()->Device()->CreateImage(
      VK_FORMAT_R32G32B32A32_SFLOAT, VkExtent2D{width, height},
      VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
      &film.result_image);
  Core()->Device()->CreateDescriptorPool(
      raytracing_film_descriptor_set_layout_->GetPoolSize(), 1,
      &film.descriptor_pool);
  film.descriptor_pool->AllocateDescriptorSet(
      raytracing_film_descriptor_set_layout_->Handle(), &film.descriptor_set);
  film.descriptor_set->BindStorageImage(0, film.result_image.get());

  Core()->Device()->NameObject(film.descriptor_set->Handle(),
                               "Ray Tracing Film Descriptor Set");

  Core()->SingleTimeCommands([image = film.result_image->Handle()](
                                 VkCommandBuffer cmd_buffer) {
    vulkan::TransitImageLayout(
        cmd_buffer, image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR, VK_ACCESS_MEMORY_READ_BIT,
        VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
  });

  pp_film.construct(std::move(film));
  return 0;
}

void Renderer::RenderScene(VkCommandBuffer cmd_buffer,
                           Film *film,
                           Scene *scene) {
  std::vector<VkClearValue> clear_values(6);
  clear_values[0].color = {0.0f, 0.0f, 0.0f, 1.0f};
  clear_values[1].color = {0.0f, 0.0f, 0.0f, 1.0f};
  clear_values[2].color = {0.0f, 0.0f, 0.0f, 1.0f};
  clear_values[3].color = {0.6f, 0.7f, 0.8f, 1.0f};
  clear_values[4].depthStencil = {1.0f, 0};
  clear_values[5].color = {-1, -1, -1, -1};
  VkRenderPassBeginInfo render_pass_begin_info{
      VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
      nullptr,
      render_pass_->Handle(),
      film->framebuffer->Handle(),
      {{0, 0}, film->framebuffer->Extent()},
      static_cast<uint32_t>(clear_values.size()),
      clear_values.data()};
  vkCmdBeginRenderPass(cmd_buffer, &render_pass_begin_info,
                       VK_SUBPASS_CONTENTS_INLINE);

  vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    envmap_pipeline_->Handle());

  // scissor and viewport
  VkViewport viewport = {0.0f,
                         0.0f,
                         static_cast<float>(film->framebuffer->Extent().width),
                         static_cast<float>(film->framebuffer->Extent().height),
                         0.0f,
                         1.0f};
  VkRect2D scissor = {{0, 0}, film->framebuffer->Extent()};
  vkCmdSetViewport(cmd_buffer, 0, 1, &viewport);
  vkCmdSetScissor(cmd_buffer, 0, 1, &scissor);

  // bind descriptor set
  VkDescriptorSet descriptor_sets[] = {
      scene->SceneSettingsDescriptorSet(core_->CurrentFrame())};
  VkDescriptorSet far_descriptor_sets[] = {
      scene->FarSceneSettingsDescriptorSet(core_->CurrentFrame())};

  vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          entity_pipeline_layout_->Handle(), 0, 1,
                          far_descriptor_sets, 0, nullptr);

  scene->DrawEnvmap(cmd_buffer, core_->CurrentFrame());

  vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    entity_pipeline_->Handle());

  scene->DrawEntities(cmd_buffer, core_->CurrentFrame());

  VkClearAttachment clearAttachment = {};
  VkClearRect clearRect = {};

  clearAttachment.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
  clearAttachment.clearValue.depthStencil = {1.0f, 0};

  // Define the area to clear (entire framebuffer in this example)
  clearRect.rect.offset = {0, 0};
  clearRect.rect.extent = film->framebuffer->Extent();
  clearRect.baseArrayLayer = 0;
  clearRect.layerCount = 1;

  // Ensure you are inside an active render pass
  vkCmdClearAttachments(cmd_buffer, 1, &clearAttachment, 1, &clearRect);

  vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          entity_pipeline_layout_->Handle(), 0, 1,
                          descriptor_sets, 0, nullptr);

  scene->DrawEntities(cmd_buffer, core_->CurrentFrame());

  vkCmdEndRenderPass(cmd_buffer);
}

void Renderer::RenderSceneRayTracing(VkCommandBuffer cmd_buffer,
                                     RayTracingFilm *film,
                                     sparks::Scene *scene) {
  vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
                    raytracing_pipeline_->Handle());

  std::vector<VkDescriptorSet> descriptor_sets{
      scene->SceneSettingsDescriptorSet(core_->CurrentFrame()),
      scene->RayTracingDescriptorSet(core_->CurrentFrame()),
      scene->Renderer()->AssetManager()->DescriptorSet(core_->CurrentFrame()),
      scene->GetEnvMap()->DescriptorSet(core_->CurrentFrame()),
      film->descriptor_set->Handle()};

  vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
                          raytracing_pipeline_layout_->Handle(), 0,
                          descriptor_sets.size(), descriptor_sets.data(), 0,
                          nullptr);

  auto aligned_size = [](uint32_t value, uint32_t alignment) {
    return (value + alignment - 1) & ~(alignment - 1);
  };

  VkPhysicalDeviceRayTracingPipelinePropertiesKHR
      ray_tracing_pipeline_properties =
          core_->Device()
              ->PhysicalDevice()
              .GetPhysicalDeviceRayTracingPipelineProperties();

  const uint32_t handle_size_aligned =
      aligned_size(ray_tracing_pipeline_properties.shaderGroupHandleSize,
                   ray_tracing_pipeline_properties.shaderGroupHandleAlignment);

  VkStridedDeviceAddressRegionKHR ray_gen_shader_sbt_entry{};
  ray_gen_shader_sbt_entry.deviceAddress =
      raytracing_sbt_->GetRayGenDeviceAddress();
  ray_gen_shader_sbt_entry.stride = handle_size_aligned;
  ray_gen_shader_sbt_entry.size = handle_size_aligned;

  VkStridedDeviceAddressRegionKHR miss_shader_sbt_entry{};
  miss_shader_sbt_entry.deviceAddress = raytracing_sbt_->GetMissDeviceAddress();
  miss_shader_sbt_entry.stride = handle_size_aligned;
  miss_shader_sbt_entry.size = handle_size_aligned;

  VkStridedDeviceAddressRegionKHR hit_shader_sbt_entry{};
  hit_shader_sbt_entry.deviceAddress =
      raytracing_sbt_->GetClosestHitDeviceAddress();
  hit_shader_sbt_entry.stride = handle_size_aligned;
  hit_shader_sbt_entry.size = handle_size_aligned;

  VkStridedDeviceAddressRegionKHR callable_shader_sbt_entry{};
  core_->Device()->Procedures().vkCmdTraceRaysKHR(
      cmd_buffer, &ray_gen_shader_sbt_entry, &miss_shader_sbt_entry,
      &hit_shader_sbt_entry, &callable_shader_sbt_entry,
      film->result_image->Extent().width, film->result_image->Extent().height,
      1);
}
}  // namespace sparks
