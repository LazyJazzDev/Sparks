#version 450

layout(set = 0, binding = 0, std140) uniform SceneSettings {
  mat4 projection;
  mat4 inv_projection;
  mat4 world_to_camera;
  mat4 camera_to_world;
}
scene_settings;

layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec3 in_tangent;
layout(location = 3) in vec2 in_tex_coord;
layout(location = 4) in float in_signal;

layout(location = 0) out vec3 out_pos;
layout(location = 1) out vec3 out_normal;
layout(location = 2) out vec3 out_tangent;
layout(location = 4) out vec3 out_bitangent;
layout(location = 3) out vec2 out_tex_coord;

void main() {
  out_pos = in_pos;
  out_normal = in_normal;
  out_tangent = in_tangent;
  out_bitangent = in_signal * cross(in_normal, in_tangent);
  out_tex_coord = in_tex_coord;
  gl_Position = scene_settings.projection * scene_settings.world_to_camera *
                vec4(in_pos, 1.0);
}
