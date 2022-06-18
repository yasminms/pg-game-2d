#version 410

in vec2 text_map;
uniform sampler2D sprite;
out vec4 frag_color;
        
void main() {
    frag_color = texture(sprite, text_map);
}
