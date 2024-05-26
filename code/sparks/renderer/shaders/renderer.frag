#version 450

layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec3 in_tangent;
layout(location = 4) in vec3 in_bitangent;
layout(location = 3) in vec2 in_tex_coord;

layout(location = 0) out vec4 out_albedo;
layout(location = 1) out vec4 out_position;
layout(location = 2) out vec4 out_normal;
layout(location = 3) out vec4 out_intensity;

void main() {
  vec3 color = vec3(1.0);
  vec3 normal = normalize(in_normal);
  vec3 tangent = normalize(in_tangent);
  vec3 bitangent = normalize(in_bitangent);

  vec3 light_dir = normalize(vec3(1.0, 1.0, 1.0));

  color *= max(dot(normal, light_dir), 0.0) * 0.7 + 0.3;
  out_albedo = vec4(1.0);
  out_position = vec4(in_pos, 0.0);
  out_normal = vec4(normal, 0.0);
  out_intensity = vec4(color, 1.0);
}
