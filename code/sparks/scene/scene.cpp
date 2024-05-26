#include "sparks/scene/scene.h"

#include "sparks/renderer/renderer.h"

namespace sparks {
Scene::Scene(struct Renderer *renderer,
             struct AssetManager *asset_manager,
             int max_entities)
    : renderer_(renderer), asset_manager_(asset_manager) {
  vulkan::DescriptorPoolSize pool_size;
  pool_size = pool_size + renderer_->SceneDescriptorSetLayout()->GetPoolSize() *
                              renderer_->Core()->MaxFramesInFlight();

  pool_size =
      pool_size + renderer_->EnvmapDescriptorSetLayout()->GetPoolSize() *
                      renderer_->Core()->MaxFramesInFlight();

  //  pool_size =
  //      pool_size + renderer_->EntityDescriptorSetLayout()->GetPoolSize() *
  //                      max_entities * renderer_->Core()->MaxFramesInFlight();

  renderer_->Core()->Device()->CreateDescriptorPool(
      pool_size, renderer_->Core()->MaxFramesInFlight() * (max_entities + 2),
      &descriptor_pool_);

  scene_settings_buffer_ =
      std::make_unique<vulkan::DynamicBuffer<SceneSettings>>(
          renderer_->Core(), 1, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

  descriptor_sets_.resize(renderer_->Core()->MaxFramesInFlight());
  for (int i = 0; i < renderer_->Core()->MaxFramesInFlight(); i++) {
    descriptor_pool_->AllocateDescriptorSet(
        renderer_->SceneDescriptorSetLayout()->Handle(), &descriptor_sets_[i]);

    descriptor_sets_[i]->BindUniformBuffer(
        0, scene_settings_buffer_->GetBuffer(i));
  }

  envmap_ = std::make_unique<EnvMap>(this);
}

Scene::~Scene() {
  envmap_.reset();
  entities_.clear();
  descriptor_sets_.clear();
  scene_settings_buffer_.reset();
  descriptor_pool_.reset();
}

int Scene::CreateEntity(Entity **pp_entity) {
  entities_[next_entity_id_] = std::make_unique<Entity>(this);
  if (pp_entity) {
    *pp_entity = entities_[next_entity_id_].get();
  }
  return next_entity_id_++;
}

void Scene::Update(float delta_time) {
  envmap_->Update();
  UpdateDynamicBuffers();
  for (auto &entity : entities_) {
    entity.second->Update();
  }
}

void Scene::SyncData(VkCommandBuffer cmd_buffer, int frame_id) {
  envmap_->Sync(cmd_buffer, frame_id);
  scene_settings_buffer_->SyncData(cmd_buffer, frame_id);
  for (auto &entity : entities_) {
    entity.second->Sync(cmd_buffer, frame_id);
  }
}

void Scene::UpdateDynamicBuffers() {
  VkExtent2D extent = renderer_->Core()->Swapchain()->Extent();
  SceneSettings scene_settings;
  scene_settings.view = camera_.GetView();
  scene_settings.projection = camera_.GetProjection(
      static_cast<float>(extent.width) / static_cast<float>(extent.height));
  scene_settings.inv_projection = glm::inverse(scene_settings.projection);
  scene_settings.inv_view = glm::inverse(scene_settings.view);
  scene_settings_buffer_->At(0) = scene_settings;
}

void Scene::DrawEnvmap(VkCommandBuffer cmd_buffer, int frame_id) {
  VkDescriptorSet descriptor_sets[] = {
      envmap_->DescriptorSet(frame_id)->Handle()};
  vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          renderer_->EnvmapPipelineLayout()->Handle(), 1, 1,
                          descriptor_sets, 0, nullptr);
  vkCmdDraw(cmd_buffer, 6, 1, 0, 0);
}

void Scene::DrawEntities(VkCommandBuffer cmd_buffer, int frame_id) {
  for (auto &entity : entities_) {
    VkDescriptorSet descriptor_sets[] = {
        entity.second->DescriptorSet(frame_id)->Handle()};
    vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            renderer_->EntityPipelineLayout()->Handle(), 1, 1,
                            descriptor_sets, 0, nullptr);

    uint32_t mesh_id = entity.second->MeshId();
    auto mesh = asset_manager_->GetMesh(mesh_id);
    VkBuffer vertex_buffers[] = {
        mesh->vertex_buffer_->GetBuffer(frame_id)->Handle()};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(cmd_buffer, 0, 1, vertex_buffers, offsets);
    vkCmdBindIndexBuffer(cmd_buffer,
                         mesh->index_buffer_->GetBuffer(frame_id)->Handle(), 0,
                         VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(cmd_buffer, mesh->index_buffer_->Length(), 2, 0, 0, 0);
  }
}
}  // namespace sparks
