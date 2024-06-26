#version 450

vec2 vertices[6] = vec2[](vec2(-1.0, -1.0),
                          vec2(1.0, -1.0),
                          vec2(1.0, 1.0),
                          vec2(-1.0, -1.0),
                          vec2(1.0, 1.0),
                          vec2(-1.0, 1.0));

void main() {
  gl_Position = vec4(vertices[gl_VertexIndex], 0.0, 1.0);
}
