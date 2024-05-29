#version 450

#define SCENE_SETTINGS_SET 0
#include "scene_settings.glsl"

layout(set = 1, binding = 0, std140) uniform ModelSettings {
  mat4 model;
  vec4 detail_scale_offset;
  vec4 color;
}
model_settings;

layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec3 in_tangent;
layout(location = 3) in vec2 in_tex_coord;
layout(location = 4) in float in_signal;

layout(location = 0) out vec3 out_pos;
layout(location = 1) out vec3 out_normal;
layout(location = 2) out vec3 out_tangent;
layout(location = 3) out vec3 out_bitangent;
layout(location = 4) out vec2 out_tex_coord;
layout(location = 5) out float out_signal;

void main() {
  out_signal = (gl_InstanceIndex == 1) ? -1.0 : 1.0;
  out_pos = vec3(model_settings.model * vec4(in_pos, 1.0)) *
            vec3(1.0, out_signal, 1.0);
  out_normal = transpose(inverse(mat3(model_settings.model))) * in_normal;
  out_tangent = mat3(model_settings.model) * in_tangent;
  out_bitangent =
      mat3(model_settings.model) * (in_signal * cross(in_normal, in_tangent));
  out_tex_coord = in_tex_coord;
  gl_Position = (scene_settings.projection * scene_settings.world_to_camera *
                 vec4(out_pos, 1.0)) *
                vec4(1.0, -1.0, 1.0, 1.0);
}
