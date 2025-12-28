#version 430 core

// Simple fullscreen vertex shader for G-Buffer testing
// Draws a fullscreen triangle without any vertex buffers

out vec2 v_TexCoord;

void main() {
    // Generate fullscreen triangle vertices from vertex ID
    // This covers the entire screen with a single triangle (more efficient than quad)
    vec2 positions[3] = vec2[](
        vec2(-1.0, -1.0),
        vec2( 3.0, -1.0),
        vec2(-1.0,  3.0)
    );
    
    vec2 texCoords[3] = vec2[](
        vec2(0.0, 0.0),
        vec2(2.0, 0.0),
        vec2(0.0, 2.0)
    );
    
    gl_Position = vec4(positions[gl_VertexID], 0.0, 1.0);
    v_TexCoord = texCoords[gl_VertexID];
}
