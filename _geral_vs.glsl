#version 410

layout ( location = 0 ) in vec2 vPosition;
layout ( location = 1 ) in vec2 vTexture;
uniform mat4 projection;
out vec2 text_map;

void main() {
    text_map = vTexture;
    gl_Position = projection * vec4 ( vPosition, 0.0, 1.0);
}
