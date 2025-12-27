#version 330 core

in vec3 v_FragPos;
in vec3 v_Normal;
in vec2 v_TexCoord;
in vec3 v_ViewPos;
in vec4 v_LightSpacePos;
in mat3 v_TBN;

out vec4 FragColor;

// Uniforms matching SceneRenderer expectations
uniform vec3 uLightDirection;
uniform vec3 uLightColor;
uniform float uLightIntensity;
uniform float uAmbientStrength;

// Material defaults (set via modelMaterial_ in RenderSystem)
uniform vec4 uDiffuseColor;

void main() {
    vec3 normal = normalize(v_Normal);
    vec3 lightDir = normalize(uLightDirection);
    
    // Use diffuse color or default gray
    vec3 baseColor = uDiffuseColor.rgb;
    if (length(baseColor) < 0.01) {
        baseColor = vec3(0.7, 0.7, 0.7);
    }
    
    // Simple lighting
    vec3 ambient = uAmbientStrength * baseColor;
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * uLightColor * uLightIntensity * baseColor;
    
    // Final color
    vec3 result = ambient + diffuse;
    FragColor = vec4(result, 1.0);
}
