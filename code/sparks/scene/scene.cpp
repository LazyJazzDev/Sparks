#include "sparks/scene/scene.h"

#include "sparks/renderer/renderer.h"

namespace sparks {
Scene::Scene(struct Renderer *renderer, int max_entities)
    : renderer_(renderer) {
  vulkan::DescriptorPoolSize pool_size;
  pool_size = pool_size + renderer_->SceneDescriptorSetLayout()->GetPoolSize() *
                              renderer_->Core()->MaxFramesInFlight();

  pool_size =
      pool_size + renderer_->EnvmapDescriptorSetLayout()->GetPoolSize() *
                      renderer_->Core()->MaxFramesInFlight();

  pool_size =
      pool_size + renderer_->EntityDescriptorSetLayout()->GetPoolSize() *
                      max_entities * renderer_->Core()->MaxFramesInFlight();

  pool_size =
      pool_size + renderer_->RayTracingDescriptorSetLayout()->GetPoolSize() *
                      renderer_->Core()->MaxFramesInFlight();

  renderer_->Core()->Device()->CreateDescriptorPool(
      pool_size, renderer_->Core()->MaxFramesInFlight() * (max_entities + 3),
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

  renderer_->Core()->CreateTopLevelAccelerationStructure({}, &top_level_as_);

  raytracing_descriptor_sets_.resize(renderer_->Core()->MaxFramesInFlight());
  for (int i = 0; i < renderer_->Core()->MaxFramesInFlight(); i++) {
    descriptor_pool_->AllocateDescriptorSet(
        renderer_->RayTracingDescriptorSetLayout()->Handle(),
        &raytracing_descriptor_sets_[i]);
  }

  envmap_ = std::make_unique<EnvMap>(this);
}

Scene::~Scene() {
  top_level_as_.reset();
  envmap_.reset();
  entities_.clear();
  descriptor_sets_.clear();
  raytracing_descriptor_sets_.clear();
  scene_settings_buffer_.reset();
  descriptor_pool_.reset();
}

int Scene::CreateEntity(Entity **pp_entity) {
  entities_[next_entity_id_] = std::make_unique<Entity>(this);
  if (pp_entity) {
    *pp_entity = entities_[next_entity_id_].get();
  }
  entities_[next_entity_id_]->metadata_.entity_id = next_entity_id_;
  return next_entity_id_++;
}

void Scene::Update(float delta_time) {
  if (update_callback_) {
    update_callback_(this, delta_time);
  }

  envmap_->Update();
  UpdateDynamicBuffers();

  for (auto &entity : entities_) {
    entity.second->Update();
  }

  std::vector<std::pair<vulkan::AccelerationStructure *, glm::mat4>> instances;
  for (auto &entity : entities_) {
    uint32_t mesh_id = entity.second->MeshId();
    auto mesh = renderer_->AssetManager()->GetMesh(mesh_id);
    instances.emplace_back(mesh->blas_.get(),
                           entity.second->metadata_.transform);
  }
  static int last_num_instances = -1;
  if (last_num_instances != instances.size()) {
    last_num_instances = instances.size();
    renderer_->Core()->Device()->WaitIdle();
    renderer_->Core()->CreateTopLevelAccelerationStructure(instances,
                                                           &top_level_as_);
  } else {
    top_level_as_->UpdateInstances(instances,
                                   renderer_->Core()->GraphicsCommandPool(),
                                   renderer_->Core()->GraphicsQueue());
  }

  std::vector<const vulkan::Buffer *> vertex_buffers;
  std::vector<const vulkan::Buffer *> index_buffers;
  for (auto mesh_id : renderer_->AssetManager()->GetMeshIds()) {
    auto mesh = renderer_->AssetManager()->GetMesh(mesh_id);
    vertex_buffers.push_back(mesh->vertex_buffer_->GetBuffer());
    index_buffers.push_back(mesh->index_buffer_->GetBuffer());
  }

  raytracing_descriptor_sets_[renderer_->Core()->CurrentFrame()]
      ->BindAccelerationStructure(0, top_level_as_.get());
  //        raytracing_descriptor_sets_[renderer_->Core()->CurrentFrame()]
  //                ->BindStorageBuffers(1, vertex_buffers);
  //        raytracing_descriptor_sets_[renderer_->Core()->CurrentFrame()]
  //                ->BindStorageBuffers(2, index_buffers);
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
    auto mesh = renderer_->AssetManager()->GetMesh(mesh_id);
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

int Scene::SetEntityTransform(uint32_t entity_id, const glm::mat4 &transform) {
  if (entities_.find(entity_id) == entities_.end()) {
    return -1;
  }
  entities_[entity_id]->metadata_.transform = transform;
  return 0;
}

int Scene::GetEntityTransform(uint32_t entity_id, glm::mat4 &transform) const {
  if (entities_.find(entity_id) == entities_.end()) {
    return -1;
  }
  transform = entities_.at(entity_id)->metadata_.transform;
  return 0;
}

int Scene::SetEntityMaterial(uint32_t entity_id, const Material &material) {
  if (entities_.find(entity_id) == entities_.end()) {
    return -1;
  }
  entities_[entity_id]->material_ = material;
  return 0;
}

int Scene::GetEntityMaterial(uint32_t entity_id, Material &material) const {
  if (entities_.find(entity_id) == entities_.end()) {
    return -1;
  }
  material = entities_.at(entity_id)->material_;
  return 0;
}
}  // namespace sparks
