#include "sparks/renderer/renderer.h"

#include "sparks/scene/scene.h"

namespace sparks {

namespace {
#include "built_in_shaders.inl"
}

Renderer::Renderer(vulkan::Core *core) : core_(core) {
  CreateDescriptorSetLayouts();
  CreateRenderPass();
  CreateEnvmapPipeline();
  CreateEntityPipeline();
}

Renderer::~Renderer() {
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
      {{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT,
        nullptr}},
      &scene_descriptor_set_layout_);
  core_->Device()->CreateDescriptorSetLayout(
      {{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT,
        nullptr},
       {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
        VK_SHADER_STAGE_FRAGMENT_BIT, &sampler}},
      &envmap_descriptor_set_layout_);
  core_->Device()->CreateDescriptorSetLayout(
      {{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1,
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
       {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
        VK_SHADER_STAGE_FRAGMENT_BIT, &sampler},
       {2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
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
      0, kIntensityFormat, VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR,
      VK_ATTACHMENT_STORE_OP_STORE, VK_ATTACHMENT_LOAD_OP_DONT_CARE,
      VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_UNDEFINED,
      VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL});
  descriptions.emplace_back(VkAttachmentDescription{
      0, kDepthFormat, VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR,
      VK_ATTACHMENT_STORE_OP_STORE, VK_ATTACHMENT_LOAD_OP_DONT_CARE,
      VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_UNDEFINED,
      VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL});

  vulkan::SubpassSettings subpass;
  subpass.color_attachment_references.push_back(
      VkAttachmentReference{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});
  subpass.color_attachment_references.push_back(
      VkAttachmentReference{1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});
  subpass.color_attachment_references.push_back(
      VkAttachmentReference{2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});
  subpass.color_attachment_references.push_back(
      VkAttachmentReference{3, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});
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

int Renderer::CreateScene(AssetManager *asset_manager,
                          int max_entities,
                          double_ptr<class Scene> pp_scene) {
  pp_scene.construct(this, asset_manager, max_entities);
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
      kIntensityFormat, VkExtent2D{width, height},
      VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
      &film.intensity_image);
  Core()->Device()->CreateImage(kDepthFormat, VkExtent2D{width, height},
                                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
                                    VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                                &film.depth_image);
  RenderPass()->CreateFramebuffer(
      {film.albedo_image->ImageView(), film.position_image->ImageView(),
       film.normal_image->ImageView(), film.intensity_image->ImageView(),
       film.depth_image->ImageView()},
      VkExtent2D{width, height}, &film.framebuffer);
  pp_film.construct(std::move(film));
  return 0;
}

void Renderer::RenderScene(VkCommandBuffer cmd_buffer,
                           Film *film,
                           Scene *scene) {
  std::vector<VkClearValue> clear_values(5);
  clear_values[0].color = {0.0f, 0.0f, 0.0f, 1.0f};
  clear_values[1].color = {0.0f, 0.0f, 0.0f, 1.0f};
  clear_values[2].color = {0.0f, 0.0f, 0.0f, 1.0f};
  clear_values[3].color = {0.6f, 0.7f, 0.8f, 1.0f};
  clear_values[4].depthStencil = {1.0f, 0};
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

  vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          entity_pipeline_layout_->Handle(), 0, 1,
                          descriptor_sets, 0, nullptr);

  scene->DrawEnvmap(cmd_buffer, core_->CurrentFrame());

  vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    entity_pipeline_->Handle());

  scene->DrawEntities(cmd_buffer, core_->CurrentFrame());

  vkCmdEndRenderPass(cmd_buffer);
}
}  // namespace sparks
