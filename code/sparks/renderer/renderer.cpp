#include "sparks/renderer/renderer.h"

#include "sparks/scene/scene.h"

namespace sparks {

namespace {
#include "built_in_shaders.inl"
}

Renderer::Renderer(vulkan::Core *core) : core_(core) {
  CreateDescriptorSetLayouts();
  CreatePipelines();
}

Renderer::~Renderer() {
  DestroyPipelines();
  DestroyDescriptorSetLayouts();
}

void Renderer::CreateDescriptorSetLayouts() {
  core_->Device()->CreateDescriptorSetLayout(
      {{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT,
        nullptr}},
      &scene_descriptor_set_layout_);
}

void Renderer::DestroyDescriptorSetLayouts() {
  scene_descriptor_set_layout_.reset();
}

void Renderer::CreatePipelines() {
  LogInfo("Creating Pipelines...");
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

  core_->Device()->CreatePipelineLayout(
      {scene_descriptor_set_layout_->Handle()}, &pipeline_layout_);

  core_->Device()->CreateShaderModule(
      vulkan::CompileGLSLToSPIRV(GetShaderCode("shaders/renderer.vert"),
                                 VK_SHADER_STAGE_VERTEX_BIT),
      &vertex_shader_);
  core_->Device()->CreateShaderModule(
      vulkan::CompileGLSLToSPIRV(GetShaderCode("shaders/renderer.frag"),
                                 VK_SHADER_STAGE_FRAGMENT_BIT),
      &fragment_shader_);

  vulkan::PipelineSettings pipeline_settings{render_pass_.get(),
                                             pipeline_layout_.get(), 0};
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

  pipeline_settings.AddShaderStage(vertex_shader_.get(),
                                   VK_SHADER_STAGE_VERTEX_BIT);
  pipeline_settings.AddShaderStage(fragment_shader_.get(),
                                   VK_SHADER_STAGE_FRAGMENT_BIT);

  pipeline_settings.SetPrimitiveTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
  pipeline_settings.SetCullMode(VK_CULL_MODE_NONE);

  core_->Device()->CreatePipeline(pipeline_settings, &pipeline_);
}

void Renderer::DestroyPipelines() {
  pipeline_.reset();

  vertex_shader_.reset();
  fragment_shader_.reset();

  pipeline_layout_.reset();
  render_pass_.reset();
}

int Renderer::CreateScene(AssetManager *asset_manager,
                          double_ptr<Scene> *pp_scene,
                          int max_entities) {
  pp_scene->construct(this, asset_manager, max_entities);
  return 0;
}
}  // namespace sparks
