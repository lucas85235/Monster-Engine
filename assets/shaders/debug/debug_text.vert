#version 330 core

layout(location = 0) in vec2 aPosition;  // 2D position offset from center
layout(location = 1) in vec2 aTexCoord;  // UV for glyph atlas

uniform mat4 uViewProjection;
uniform vec3 uWorldPos;      // Billboard center in world space
uniform vec3 uCameraRight;   // Camera right vector
uniform vec3 uCameraUp;      // Camera up vector
uniform float uScale;        // Text scale

out vec2 vTexCoord;

void main() {
    // Billboard position: offset from world position using camera-aligned axes
    vec3 worldPos = uWorldPos 
                  + uCameraRight * aPosition.x * uScale
                  + uCameraUp * aPosition.y * uScale;
    
    gl_Position = uViewProjection * vec4(worldPos, 1.0);
    vTexCoord = aTexCoord;
}
