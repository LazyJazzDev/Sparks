#ifndef SCENE_SETTINGS_H
#define SCENE_SETTINGS_H

struct SceneSettings {
  mat4 projection;
  mat4 inv_projection;
  mat4 world_to_camera;
  mat4 camera_to_world;
  float gamma;
  float exposure;
  float persistence;
  uint accumulated_sample;
  uint num_sample;
  uint num_bounces;
  float clamp_value;
  float total_emission_energy;
  uint num_entity;
};

#endif
