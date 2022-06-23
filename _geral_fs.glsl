#version 410

in vec2 text_map;
out vec4 frag_color;

uniform sampler2D sprite;
uniform float sprite_offset_x;
uniform float sprite_offset_y;
        
void main() {
    vec4 texel = texture(sprite, vec2(text_map.x + sprite_offset_x, text_map.y + sprite_offset_y));
    
    if (texel.a < 0.5) {
        discard;
    }

    frag_color = texel;
}
