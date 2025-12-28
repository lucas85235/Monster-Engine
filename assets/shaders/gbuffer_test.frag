#version 430 core

// Simple fullscreen shader that outputs to G-Buffer for testing.
// This is used to verify the G-Buffer FBO is working correctly.

layout(location = 0) out vec3 gPosition;
layout(location = 1) out vec3 gNormal;
layout(location = 2) out vec4 gAlbedo;
layout(location = 3) out vec3 gEmissive;

in vec2 v_TexCoord;

void main() {
    // Output world position as UV coordinates for visualization
    gPosition = vec3(v_TexCoord * 10.0, 0.0);
    
    // Point normal up
    gNormal = vec3(0.0, 1.0, 0.0);
    
    // White albedo
    gAlbedo = vec4(1.0, 1.0, 1.0, 1.0);
    
    // BRIGHT MAGENTA EMISSIVE - gradient based on screen position
    // This should be very visible in the debug view
    float emissiveX = v_TexCoord.x * 2.0;
    float emissiveY = v_TexCoord.y * 2.0;
    gEmissive = vec3(emissiveX, 0.0, emissiveY);
}
