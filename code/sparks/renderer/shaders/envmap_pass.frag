#version 450

layout(location = 0) in vec3 ray_direction;

layout(set = 1, binding = 0, std140) uniform EnvmapData {
  float envmap_rotation;
  float exposure;
  bool reflect;
}
envmap_data;

layout(set = 1, binding = 1) uniform sampler2D envmap;

layout(location = 0) out vec4 out_albedo;
layout(location = 1) out vec4 out_position;
layout(location = 2) out vec4 out_normal;
layout(location = 3) out vec4 out_color;

void main() {
  const float pi = 3.1415926535897932384626433832795;
  vec3 direction = normalize(ray_direction);
  direction.y = acos(direction.y) / pi;
  direction.x = atan(direction.x, direction.z) / (2.0 * pi);
  direction = direction + vec3(0.5, 0.0, 0.0);

  if (envmap_data.reflect && direction.y > 0.5) {
    direction.y = 1.0 - direction.y;
  }

  vec3 color = texture(envmap, direction.xy).rgb;
  color = color * envmap_data.exposure;
  out_albedo = vec4(0.0);
  out_position = vec4(0.0);
  out_normal = vec4(-direction, 0.0);
  out_color = vec4(color, 1.0);
}
