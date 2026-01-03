#version 330 core

in vec2 vTexCoord;

uniform sampler2D uFontAtlas;
uniform vec3 uColor;

out vec4 FragColor;

void main() {
    // Sample font atlas (only red channel contains the glyph)
    float alpha = texture(uFontAtlas, vTexCoord).r;
    
    if (alpha < 0.1) {
        discard;
    }
    
    FragColor = vec4(uColor, alpha);
}
