#ifndef ENVMAP_DIRECT_LIGHTING_GLSL
#define ENVMAP_DIRECT_LIGHTING_GLSL

#include "envmap.glsl"
#include "hit_record.glsl"
#include "random.glsl"
#include "shadow_ray.glsl"
#include "trace_ray.glsl"

float PowerHeuristic(float base, float ref) {
  return (base * base) / (base * base + ref * ref);
}

ivec2 EnvmapSize() {
  return ivec2(textureSize(
      sampler2D(sampled_textures[envmap_data.envmap_id], samplers[0]), 0));
}

vec3 EnvmapSample(vec3 direction) {
  return texture(
             sampler2D(sampled_textures[envmap_data.envmap_id], samplers[0]),
             SampleEnvmapUV(direction))
             .xyz *
         envmap_data.scale;
}

void EnvmapSampleDirectionLighting(inout vec3 eval,
                                   inout vec3 omega_in,
                                   inout float pdf,
                                   float r1) {
  ivec2 envmap_size = EnvmapSize();
  int L = 0, R = envmap_size.x * envmap_size.y - 1;
  while (L < R) {
    int mid = (L + R) / 2;
    if (envmap_cdf[mid] < r1) {
      L = mid + 1;
    } else {
      R = mid;
    }
  }
  float pixel_prob = envmap_cdf[L];
  if (L > 0) {
    r1 -= envmap_cdf[L - 1];
    pixel_prob -= envmap_cdf[L - 1];
  }
  r1 /= pixel_prob;
  int y = L / envmap_size.x;
  int x = L - y * envmap_size.x;
  float inv_width = 1.0 / float(envmap_size.x);
  float inv_height = 1.0 / float(envmap_size.y);
  float z_lbound = cos(y * inv_height * PI);
  float z_ubound = cos((y + 1) * inv_height * PI);
  vec2 uv = vec2((x + r1) * inv_width,
                 acos(mix(z_lbound, z_ubound, RandomFloat())) * INV_PI);
  z_lbound = 0.5 * (1.0 - z_lbound);
  z_ubound = 0.5 * (1.0 - z_ubound);
  omega_in = SampleEnvmapDirection(uv);
  float shadow = ShadowRay(hit_record.position, omega_in, 1e4);
  if (shadow > 1e-4) {
    vec3 color = EnvmapSample(omega_in).xyz;
    pdf = pixel_prob / ((z_ubound - z_lbound) * inv_width);
    eval = shadow * color * 4 * PI / pdf;
  }
}

float EstimateEnvmapDirectLightingPdf() {
  float pdf = 0.0;
  if (ray_payload.t == -1.0) {
    ivec2 envmap_size = EnvmapSize();
    vec2 uv = SampleEnvmapUV(trace_ray_direction);
    ivec2 uv_coord = ivec2(uv * envmap_size);
    uv_coord.x = uv_coord.x % envmap_size.x;
    uv_coord.y = uv_coord.y % envmap_size.y;
    int idx = uv_coord.y * envmap_size.x + uv_coord.x;
    float pixel_prob = envmap_cdf[idx];
    float inv_height = 1.0 / float(envmap_size.y);
    if (idx > 0) {
      pixel_prob -= envmap_cdf[idx - 1];
    }
    float z_lbound = cos(uv_coord.y * inv_height * PI);
    float z_ubound = cos((uv_coord.y + 1) * inv_height * PI);
    pdf = pixel_prob * envmap_size.x / (0.5 * (z_lbound - z_ubound));
  }
  return pdf;
}

void SampleEnvmapDirectLighting(out vec3 eval,
                                out vec3 omega_in,
                                out float pdf) {
  eval = vec3(0.0);
  omega_in = vec3(0.0);
  pdf = 0.0;

  float r1 = RandomFloat();
  EnvmapSampleDirectionLighting(eval, omega_in, pdf, r1);
}

#endif
