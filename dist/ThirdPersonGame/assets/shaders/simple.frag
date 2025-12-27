#version 330 core

in vec3 v_Color;
in vec3 v_ViewPos;
in vec3 v_Normal;
in vec3 v_FragPos;
in vec4 v_LightSpacePos;
in float f_SpecularStrenght;

uniform vec3 uLightDirection;
uniform vec3 uLightColor;
uniform float uLightIntensity;
uniform float uAmbientStrength;
uniform sampler2D uShadowMap;
uniform float uReceiveShadows;
uniform float uShadowsEnabled;

out vec4 out_FragColor;

float ShadowCalculation(vec4 lightSpacePos) {
    if (uShadowsEnabled < 0.5 || uReceiveShadows < 0.5) return 0.0;
    
    vec3 projCoords = lightSpacePos.xyz / lightSpacePos.w;
    projCoords = projCoords * 0.5 + 0.5;
    
    if (projCoords.z > 1.0 || projCoords.x < 0.0 || projCoords.x > 1.0 || 
        projCoords.y < 0.0 || projCoords.y > 1.0) {
        return 0.0;
    }
    
    float currentDepth = projCoords.z;
    float bias = max(0.005 * (1.0 - dot(normalize(v_Normal), normalize(uLightDirection))), 0.001);
    
    // PCF
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(uShadowMap, 0);
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            float pcfDepth = texture(uShadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0;
    
    return shadow;
}

void main() {
    vec3 norm = normalize(v_Normal);
    vec3 lightDir = normalize(uLightDirection);
    
    // Ambient
    vec3 ambient = uAmbientStrength * v_Color;
    
    // Diffuse
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * uLightColor * uLightIntensity * v_Color;
    
    // Specular
    vec3 viewDir = normalize(v_ViewPos - v_FragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(norm, halfwayDir), 0.0), 32.0);
    vec3 specular = f_SpecularStrenght * spec * uLightColor * uLightIntensity;
    
    // Shadow
    float shadow = ShadowCalculation(v_LightSpacePos);
    
    vec3 result = ambient + (1.0 - shadow) * (diffuse + specular);
    out_FragColor = vec4(result, 1.0);
}
