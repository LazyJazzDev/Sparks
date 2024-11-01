#ifndef ENTITY_DIRECT_LIGHTING_GLSL
#define ENTITY_DIRECT_LIGHTING_GLSL

#include "entity_metadata.glsl"
#include "material.glsl"
#include "mesh_metadata.glsl"
#include "shadow_ray.glsl"
#include "vertex.glsl"

float EntityCDF(int x) {
  if (x < 0)
    return 0.0;
  return metadatas[x].emission_cdf;
}

float MeshCDF(uint mesh_id, int primitive_id) {
  if (primitive_id < 0)
    return 0.0;
  return mesh_cdf_buffers[mesh_id].area_cdfs[primitive_id];
}

void SampleEntityDirectLighting(out vec3 eval,
                                out vec3 omega_in,
                                out float pdf) {
  pdf = 0.0;
  eval = vec3(0.0);
  omega_in = vec3(0.0);
  float select_prob = 1.0;
  float r1 = RandomFloat();
  int L = 0, R = int(scene_settings.num_entity - 1);
  while (L < R) {
    int m = (L + R) / 2;
    if (r1 <= EntityCDF(m)) {
      R = m;
    } else {
      L = m + 1;
    }
  }
  int entity_id = L;
  vec3 emission =
      materials[entity_id].emission * materials[entity_id].emission_strength;
  uint mesh_id = metadatas[entity_id].mesh_id;
  mat4 entity_transform = metadatas[entity_id].model;
  select_prob = EntityCDF(L) - EntityCDF(L - 1);
  r1 = (r1 - EntityCDF(L - 1)) / select_prob;
  L = 0;
  R = int(mesh_metadatas[mesh_id].num_index / 3) - 1;
  while (L < R) {
    int m = (L + R) / 2;
    if (r1 <= MeshCDF(mesh_id, m)) {
      R = m;
    } else {
      L = m + 1;
    }
  }
  int primitive_id = L;
  float primitive_select_prob = MeshCDF(mesh_id, L) - MeshCDF(mesh_id, L - 1);
  r1 = (r1 - MeshCDF(mesh_id, L - 1)) / primitive_select_prob;
  select_prob *= primitive_select_prob;
  uint iu, iv, iw;
  iu = index_buffers[mesh_id].indices[primitive_id * 3 + 0];
  iv = index_buffers[mesh_id].indices[primitive_id * 3 + 1];
  iw = index_buffers[mesh_id].indices[primitive_id * 3 + 2];
  vec3 pu, pv, pw;
  pu = GetVertexPos(mesh_id, iu);
  pv = GetVertexPos(mesh_id, iv);
  pw = GetVertexPos(mesh_id, iw);
  pu = vec3(entity_transform * vec4(pu, 1.0));
  pv = vec3(entity_transform * vec4(pv, 1.0));
  pw = vec3(entity_transform * vec4(pw, 1.0));
  vec3 normal = cross(pv - pu, pw - pu);
  float area = length(normal);
  normal /= area;
  area *= 0.5;
  float r2 = RandomFloat();
  if (r1 + r2 > 1.0) {
    r1 = 1.0 - r1;
    r2 = 1.0 - r2;
  }
  vec3 hit_pos = pu + r1 * (pv - pu) + r2 * (pw - pu);
  omega_in = hit_pos - hit_record.position;
  float dist = length(omega_in);
  omega_in /= dist;
  if (dot(normal, omega_in) > 0.0) {
    normal = -normal;
  }
  float shadow = ShadowRay(hit_record.position, omega_in, dist * 0.9999);
  float cos_theta = -dot(normal, omega_in);
  if (shadow > 1e-4 && cos_theta > 1e-6) {
    pdf = dist * dist * select_prob / (area * cos_theta);
    eval = shadow * emission / pdf;
  }
}

float EstimateEntityDirectLightingPdf(vec3 origin) {
  uint mesh_id = metadatas[hit_record.entity_id].mesh_id;
  mat4 entity_transform = metadatas[hit_record.entity_id].model;
  uint iu, iv, iw;
  iu = index_buffers[mesh_id].indices[ray_payload.primitive_id * 3 + 0];
  iv = index_buffers[mesh_id].indices[ray_payload.primitive_id * 3 + 1];
  iw = index_buffers[mesh_id].indices[ray_payload.primitive_id * 3 + 2];
  vec3 pu, pv, pw;
  pu = GetVertexPos(mesh_id, iu);
  pv = GetVertexPos(mesh_id, iv);
  pw = GetVertexPos(mesh_id, iw);
  pu = vec3(entity_transform * vec4(pu, 1.0));
  pv = vec3(entity_transform * vec4(pv, 1.0));
  pw = vec3(entity_transform * vec4(pw, 1.0));
  vec3 normal = cross(pv - pu, pw - pu);
  float area = length(normal);
  area *= 0.5;
  vec3 omega_in = hit_record.position - origin;
  float dist = length(omega_in);
  omega_in /= dist;
  float select_prob = (EntityCDF(int(hit_record.entity_id)) -
                       EntityCDF(int(hit_record.entity_id - 1))) *
                      (MeshCDF(mesh_id, int(ray_payload.primitive_id)) -
                       MeshCDF(mesh_id, int(ray_payload.primitive_id - 1)));
  return dist * dist * select_prob / area /
         -dot(hit_record.geometry_normal, omega_in);
}

#endif
