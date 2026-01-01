#version 430 core

out float FragColor;
in vec2 vTexCoord;

uniform sampler2D uDepthTexture;
uniform sampler2D uNormalTexture;
uniform sampler2D uNoiseTexture;

uniform vec3 uSamples[64];
uniform mat4 uProjection;
uniform mat4 uView;
uniform vec2 uNoiseScale;
uniform float uRadius;
uniform float uBias;
uniform float uIntensity;
uniform int uKernelSize;

float linearizeDepth(float depth) {
    float near = 0.1;
    float far = 1000.0;
    float z = depth * 2.0 - 1.0;
    return (2.0 * near * far) / (far + near - z * (far - near));
}

vec3 reconstructPosition(vec2 uv, float depth) {
    vec4 clipPos = vec4(uv * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    mat4 invProj = inverse(uProjection);
    vec4 viewPos = invProj * clipPos;
    return viewPos.xyz / viewPos.w;
}

void main() {
    float depth = texture(uDepthTexture, vTexCoord).r;
    
    if (depth >= 1.0) {
        FragColor = 1.0;
        return;
    }
    
    vec3 fragPos = reconstructPosition(vTexCoord, depth);
    vec3 normal = texture(uNormalTexture, vTexCoord).rgb;
    
    if (length(normal) < 0.1) {
        normal = vec3(0.0, 1.0, 0.0);
    } else {
        normal = normalize(normal * 2.0 - 1.0);
        normal = mat3(uView) * normal;
    }
    
    vec3 randomVec = normalize(texture(uNoiseTexture, vTexCoord * uNoiseScale).xyz);
    
    vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN = mat3(tangent, bitangent, normal);
    
    float occlusion = 0.0;
    int validSamples = 0;
    
    for (int i = 0; i < uKernelSize; ++i) {
        vec3 samplePos = TBN * uSamples[i];
        samplePos = fragPos + samplePos * uRadius;
        
        vec4 offset = uProjection * vec4(samplePos, 1.0);
        offset.xyz /= offset.w;
        offset.xyz = offset.xyz * 0.5 + 0.5;
        
        if (offset.x < 0.0 || offset.x > 1.0 || offset.y < 0.0 || offset.y > 1.0) {
            continue;
        }
        
        float sampleDepth = texture(uDepthTexture, offset.xy).r;
        vec3 sampleWorldPos = reconstructPosition(offset.xy, sampleDepth);
        
        float rangeCheck = smoothstep(0.0, 1.0, uRadius / abs(fragPos.z - sampleWorldPos.z));
        occlusion += (sampleWorldPos.z >= samplePos.z + uBias ? 1.0 : 0.0) * rangeCheck;
        validSamples++;
    }
    
    if (validSamples > 0) {
        occlusion = 1.0 - (occlusion / float(validSamples));
    } else {
        occlusion = 1.0;
    }
    
    FragColor = pow(occlusion, uIntensity);
}
