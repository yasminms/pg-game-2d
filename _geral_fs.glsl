#version 410

in vec2 text_map;
out vec4 frag_color;

uniform sampler2D sprite;
uniform float sprite_offset_y;
        
void main() {
    frag_color = texture(sprite, vec2(text_map.x, text_map.y + sprite_offset_y));
}
