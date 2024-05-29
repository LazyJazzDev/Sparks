#version 460
#extension GL_EXT_ray_tracing : enable

layout(location = 0) rayPayloadInEXT vec3 colorValue;

void main() {
  colorValue = vec3(0.8, 0.7, 0.6);
}
