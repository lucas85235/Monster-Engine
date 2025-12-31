// =============================================================================
// PBR Types - Core Data Structures
// Based on Google Filament - See pbr_document.md for reference
// =============================================================================

#ifndef PBR_TYPES_GLSL
#define PBR_TYPES_GLSL

// =============================================================================
// MaterialInputs
// =============================================================================
// Artist-friendly material parameters. The pipeline converts these to PixelParams.

struct MaterialInputs {
    vec4  baseColor;           // Base color RGBA (linear)
    float roughness;           // Perceptual roughness [0-1]
    float metallic;            // Metallic factor [0-1]
    float reflectance;         // Dielectric reflectance [0-1], default 0.5 = 4% F0
    float ambientOcclusion;    // AO [0-1]
    vec4  emissive;            // Emission RGB + exposure attenuation factor
    vec3  normal;              // Normal map (tangent space)
    
    // Clear coat
    float clearCoat;
    float clearCoatRoughness;
    
    // Anisotropy (brushed metal, hair)
    float anisotropy;          // [-1, 1]
    vec3  anisotropyDirection;
    
    // Sheen (fabric, velvet)
    vec3  sheenColor;
    float sheenRoughness;
    
    // Subsurface (skin, wax, leaves)
    float subsurfacePower;
    vec3  subsurfaceColor;
    float thickness;           // [0-1]
    
    // Transmission (glass, water)
    float transmission;
    vec3  absorption;
    float ior;                 // Index of refraction, default 1.5
};

MaterialInputs initMaterialInputs() {
    MaterialInputs m;
    m.baseColor = vec4(1.0);
    m.roughness = 0.5;
    m.metallic = 0.0;
    m.reflectance = 0.5;
    m.ambientOcclusion = 1.0;
    m.emissive = vec4(0.0);
    m.normal = vec3(0.0, 0.0, 1.0);
    m.clearCoat = 0.0;
    m.clearCoatRoughness = 0.0;
    m.anisotropy = 0.0;
    m.anisotropyDirection = vec3(1.0, 0.0, 0.0);
    m.sheenColor = vec3(0.0);
    m.sheenRoughness = 0.0;
    m.subsurfacePower = 0.0;
    m.subsurfaceColor = vec3(0.0);
    m.thickness = 0.0;
    m.transmission = 0.0;
    m.absorption = vec3(0.0);
    m.ior = 1.5;
    return m;
}

// =============================================================================
// PixelParams
// =============================================================================
// Computed parameters from MaterialInputs. Used internally by the shading pipeline.

struct PixelParams {
    vec3  diffuseColor;            // baseColor * (1 - metallic)
    float perceptualRoughness;
    vec3  f0;                      // Reflectance at normal incidence
    float roughness;               // perceptualRoughness^2
    vec3  dfg;                     // Prefiltered DFG from LUT
    vec3  energyCompensation;      // Multi-scattering compensation (Kulla-Conty)
    float f90;
    
    // Clear coat
    float clearCoat;
    float clearCoatRoughness;
    
    // Sheen
    vec3  sheenColor;
    float sheenRoughness;
    float sheenScaling;
    float sheenDFG;
    
    // Anisotropy
    vec3  anisotropicT;
    vec3  anisotropicB;
    float anisotropy;
    float at;                      // Roughness along tangent
    float ab;                      // Roughness along bitangent
    
    // Subsurface/Transmission
    float subsurfacePower;
    vec3  subsurfaceColor;
    float thickness;
    float etaIR;                   // IOR air -> material
    float etaRI;                   // IOR material -> air
    float transmission;
    vec3  absorption;
};

// =============================================================================
// Light
// =============================================================================
// Unified light structure for directional, point, and spot lights.

#define LIGHT_TYPE_DIRECTIONAL 0u
#define LIGHT_TYPE_POINT       1u
#define LIGHT_TYPE_SPOT        2u

struct Light {
    vec4  colorIntensity;     // RGB + pre-exposed intensity
    vec3  l;                  // Light direction (normalized, points to light)
    float attenuation;        // Distance/angle attenuation
    vec3  worldPosition;
    float NoL;                // dot(normal, light)
    vec3  direction;          // Spotlight direction
    float zLight;             // Light-space depth for shadows
    uint  lightType;
    int   shadowIndex;        // -1 = no shadow
    bool  castsShadows;
    bool  contactShadows;
    int   channels;
};

Light createDirectionalLight(vec3 direction, vec3 color, float intensity) {
    Light light;
    light.l = normalize(direction);
    light.colorIntensity = vec4(color, intensity);
    light.attenuation = 1.0;
    light.worldPosition = vec3(0.0);
    light.NoL = 0.0;
    light.direction = light.l;
    light.zLight = 0.0;
    light.lightType = LIGHT_TYPE_DIRECTIONAL;
    light.shadowIndex = -1;
    light.castsShadows = false;
    light.contactShadows = false;
    light.channels = 0xFF;
    return light;
}

// =============================================================================
// ShadingData
// =============================================================================
// Per-fragment shading info.

struct ShadingData {
    vec3  position;
    vec3  normal;
    vec3  view;
    vec3  reflected;
    float NoV;
    vec3  tangent;
    vec3  bitangent;
};

ShadingData initShadingData(vec3 position, vec3 normal, vec3 viewPos) {
    ShadingData s;
    s.position = position;
    s.normal = normalize(normal);
    s.view = normalize(viewPos - position);
    s.reflected = reflect(-s.view, s.normal);
    s.NoV = abs(dot(s.normal, s.view)) + 1e-4;
    s.tangent = vec3(1.0, 0.0, 0.0);
    s.bitangent = vec3(0.0, 1.0, 0.0);
    return s;
}

#endif // PBR_TYPES_GLSL
