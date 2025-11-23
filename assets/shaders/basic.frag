#version 330 core
layout(location = 0) out vec4 color;

in vec3  v_Color;
in vec3  v_ViewPos;
in vec3  v_Normal;
in vec3  v_FragPos;
in vec4  v_LightSpacePos;
in float f_SpecularStrenght;

uniform vec3      uLightDirection;
uniform vec3      uLightColor;
uniform float     uLightIntensity;
uniform float     uAmbientStrength;
uniform sampler2D uShadowMap;
uniform float     uReceiveShadows;
uniform float     uShadowsEnabled;

vec3 Saturate(vec3 value) {
    return vec3(clamp(value.x, 0.0, 1.0), clamp(value.y, 0.0, 1.0), clamp(value.z, 0.0, 1.0));
}

float CalculateShadow(vec4 lightSpacePos, vec3 normal, vec3 lightDir) {
    vec3 projCoords = lightSpacePos.xyz / lightSpacePos.w;
    projCoords      = projCoords * 0.5 + 0.5;

    if (projCoords.z > 1.0 || projCoords.z < 0.0) return 0.0;

    if (projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0) return 0.0;

    float ndotl     = max(dot(normal, lightDir), 0.0);
    float bias      = max(0.000025, 0.0005 * (1.0 - ndotl));
    float shadow    = 0.0;
    vec2  texelSize = 1.0 / textureSize(uShadowMap, 0);
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            float pcfDepth = texture(uShadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += projCoords.z - bias > pcfDepth ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0;
    return shadow;
}

void main() {
    vec3  normal      = normalize(v_Normal);
    vec3  lightDir    = normalize(uLightDirection);
    vec3  objectColor = v_Color;
    float diff        = max(dot(normal, lightDir), 0.0);

    float shadow = (uReceiveShadows > 0.5 && uShadowsEnabled > 0.5) ? CalculateShadow(v_LightSpacePos, normal, lightDir) : 0.0;

    // reflection
    vec3  viewDir    = normalize(v_ViewPos - v_FragPos);
    vec3  reflectDir = reflect(-lightDir, normal);
    float spec       = pow(max(dot(viewDir, reflectDir), 0.0), 256);
    vec3  specular   = f_SpecularStrenght * spec * uLightColor;

    vec3 lightRadiance = uLightColor * max(uLightIntensity, 0.0);
    vec3 ambient       = uLightColor * uAmbientStrength;
    vec3 diffuse       = uLightColor * diff;

    // shadow strength
    shadow = mix(0.0, 0.7, shadow);

    // final light result
    vec3 result = (Saturate(ambient + diffuse) + specular) * objectColor * (1.0 - shadow);

    color = vec4(result, 1.0);
}
