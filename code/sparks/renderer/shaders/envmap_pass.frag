#version 450

layout(location = 0) in vec3 ray_direction;

layout(set = 1, binding = 0, std140) uniform EnvmapData {
  float envmap_rotation;
  float exposure;
}
envmap_data;

layout(set = 1, binding = 1) uniform sampler2D envmap;

layout(location = 3) out vec4 out_color;

void main() {
  const float pi = 3.1415926535897932384626433832795;
  vec3 direction = normalize(ray_direction);
  direction.y = acos(direction.y) / pi;
  direction.x = atan(direction.x, direction.z) / (2.0 * pi);
  direction = direction + vec3(0.5, 0.0, 0.0);

  vec3 color = texture(envmap, direction.xy).rgb;
  color = color * envmap_data.exposure;
  out_color = vec4(color, 1.0);
}
