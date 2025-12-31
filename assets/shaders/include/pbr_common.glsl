// =============================================================================
// PBR Common Functions - Based on Google's Filament Standard Model
// =============================================================================
// This file contains the core BRDF functions for physically-based rendering.
// Include this file in all PBR shaders.
// =============================================================================

#ifndef PBR_COMMON_GLSL
#define PBR_COMMON_GLSL

// -----------------------------------------------------------------------------
// Constants
// -----------------------------------------------------------------------------
const float PI = 3.14159265359;
const float MIN_ROUGHNESS = 0.045;  // Clamp to avoid precision issues
const float MIN_N_DOT_V = 1e-4;     // Avoid division by zero

// -----------------------------------------------------------------------------
// Utility Functions
// -----------------------------------------------------------------------------

float saturate(float x) {
    return clamp(x, 0.0, 1.0);
}

vec3 saturate3(vec3 x) {
    return clamp(x, vec3(0.0), vec3(1.0));
}

// -----------------------------------------------------------------------------
// Tone Mapping and Color Correction
// -----------------------------------------------------------------------------

// Apply exposure to HDR color
vec3 applyExposure(vec3 color, float exposure) {
    return color * exposure;
}

// ACES Filmic Tone Mapping (industry standard, approximation by Krzysztof Narkowicz)
// More accurate color reproduction than Reinhard, better handling of bright colors
vec3 toneMapACES(vec3 x) {
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return saturate3((x * (a * x + b)) / (x * (c * x + d) + e));
}

// Reinhard Tone Mapping (simple, good for real-time)
vec3 toneMapReinhard(vec3 color) {
    return color / (color + vec3(1.0));
}

// Extended Reinhard with white point control
vec3 toneMapReinhardExtended(vec3 color, float whitePoint) {
    vec3 numerator = color * (1.0 + color / (whitePoint * whitePoint));
    return numerator / (1.0 + color);
}

// Uncharted 2 Filmic Tone Mapping (good for games)
vec3 toneMapUncharted2Partial(vec3 x) {
    const float A = 0.15;
    const float B = 0.50;
    const float C = 0.10;
    const float D = 0.20;
    const float E = 0.02;
    const float F = 0.30;
    return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}

vec3 toneMapUncharted2(vec3 color, float exposureBias) {
    const float W = 11.2;
    vec3 curr = toneMapUncharted2Partial(color * exposureBias);
    vec3 whiteScale = vec3(1.0) / toneMapUncharted2Partial(vec3(W));
    return curr * whiteScale;
}

// Linear to sRGB gamma correction (approximate)
vec3 linearToSRGB(vec3 linearColor) {
    return pow(linearColor, vec3(1.0 / 2.2));
}

// sRGB to Linear (for texture sampling)
vec3 sRGBToLinear(vec3 srgbColor) {
    return pow(srgbColor, vec3(2.2));
}

// More accurate sRGB conversion
vec3 linearToSRGBAccurate(vec3 linear) {
    vec3 higher = vec3(1.055) * pow(linear, vec3(1.0 / 2.4)) - vec3(0.055);
    vec3 lower = linear * vec3(12.92);
    return mix(higher, lower, lessThanEqual(linear, vec3(0.0031308)));
}

// -----------------------------------------------------------------------------
// Dithering Functions (Banding Reduction)
// -----------------------------------------------------------------------------

// Interleaved Gradient Noise - Jimenez 2014
// High quality, good spatial distribution, fast computation
float interleavedGradientNoise(vec2 screenPos) {
    vec3 magic = vec3(0.06711056, 0.00583715, 52.9829189);
    return fract(magic.z * fract(dot(screenPos, magic.xy)));
}

// Triangular-distribution dithering (better than uniform for color)
// Maps [0,1] noise to [-0.5, 0.5] with triangular distribution
float triangularDither(float noise) {
    float n = noise * 2.0 - 1.0;
    return sign(n) * (1.0 - sqrt(1.0 - abs(n))) * 0.5;
}

// Apply dithering to color to reduce 8-bit banding
// The dither magnitude is 1/255 (one LDR step)
vec3 ditherOutput(vec3 color, vec2 screenPos) {
    float noise = interleavedGradientNoise(screenPos);
    float dither = triangularDither(noise);
    return color + vec3(dither / 255.0);
}

// -----------------------------------------------------------------------------
// Final Color Output Pipeline
// -----------------------------------------------------------------------------

// Complete HDR to LDR pipeline: exposure -> tone mapping -> gamma
// Legacy version without dithering (for compatibility)
vec3 finalColorOutput(vec3 hdrColor, float exposure) {
    vec3 exposed = applyExposure(hdrColor, exposure);
    vec3 tonemapped = toneMapACES(exposed);
    vec3 gammaCorrected = linearToSRGB(tonemapped);
    return gammaCorrected;
}

// HDR output for post-processing pipeline - NO tonemapping, just linear HDR
// Use this when rendering to HDR framebuffer that will be post-processed
vec3 hdrOutput(vec3 hdrColor) {
    return max(hdrColor, vec3(0.0));
}

// Complete HDR to LDR pipeline with dithering: exposure -> tone mapping -> gamma -> dither
// When uPostProcessEnabled is true, skip tonemapping and gamma - just output HDR
vec3 finalColorOutputDithered(vec3 hdrColor, float exposure, vec2 screenPos) {
    // Always output raw HDR - post-processing handles tonemapping now
    return max(hdrColor, vec3(0.0));
}

// Compute the squared perceptual roughness (alpha) from perceptual roughness
float perceptualRoughnessToRoughness(float perceptualRoughness) {
    return perceptualRoughness * perceptualRoughness;
}

// Clamp roughness to minimum value to avoid specular artifacts
float clampRoughness(float roughness) {
    return max(roughness, MIN_ROUGHNESS);
}

// -----------------------------------------------------------------------------
// Material Parameter Remapping (following Filament's approach)
// -----------------------------------------------------------------------------

// Computes diffuse color from base color and metallic factor
// Metals have no diffuse component, dielectrics keep their base color
vec3 computeDiffuseColor(vec3 baseColor, float metallic) {
    return baseColor * (1.0 - metallic);
}

// Computes dielectric F0 from reflectance parameter
// reflectance = 0.5 gives 4% F0 (most common dielectrics)
float computeDielectricF0(float reflectance) {
    return 0.16 * reflectance * reflectance;
}

// Computes F0 (specular reflectance at normal incidence)
// Dielectrics use reflectance param, metals use baseColor
vec3 computeF0(vec3 baseColor, float metallic, float reflectance) {
    return mix(vec3(computeDielectricF0(reflectance)), baseColor, metallic);
}

// -----------------------------------------------------------------------------
// Specular D - GGX Normal Distribution Function
// -----------------------------------------------------------------------------
// The GGX/Trowbridge-Reitz distribution for microfacet normals.
// Returns the probability that microfacets are oriented with the half-vector.

float D_GGX(float NoH, float roughness) {
    float a = NoH * roughness;
    float denom = max(1.0 - NoH * NoH + a * a, 1e-6);
    float k = roughness / denom;
    return k * k * (1.0 / PI);
}

// GGX with Lagrange identity for better precision (Filament)
float D_GGX_Precise(float roughness, float NoH, vec3 n, vec3 h) {
    vec3 NxH = cross(n, h);
    float a = NoH * roughness;
    float k = roughness / (dot(NxH, NxH) + a * a);
    return k * k * (1.0 / PI);
}

// Anisotropic GGX NDF - Burley 2012
float D_GGX_Anisotropic(float at, float ab, float ToH, float BoH, float NoH) {
    float a2 = at * ab;
    highp vec3 d = vec3(ab * ToH, at * BoH, a2 * NoH);
    highp float d2 = max(dot(d, d), 1e-6);
    float b2 = a2 / d2;
    return a2 * b2 * b2 * (1.0 / PI);
}

// Charlie NDF for sheen/cloth - Estevez and Kulla 2017
float D_Charlie(float roughness, float NoH) {
    float invAlpha = 1.0 / roughness;
    float cos2h = NoH * NoH;
    float sin2h = max(1.0 - cos2h, 0.0078125);
    return (2.0 + invAlpha) * pow(sin2h, invAlpha * 0.5) / (2.0 * PI);
}

// -----------------------------------------------------------------------------
// Specular V - Smith-GGX Height-Correlated Visibility Function
// -----------------------------------------------------------------------------
// Incorporates geometric shadowing/masking into a visibility term.
// This is the height-correlated Smith function which is more accurate.

float V_SmithGGXCorrelated(float NoV, float NoL, float roughness) {
    float a2 = roughness * roughness;
    float GGXV = NoL * sqrt(NoV * NoV * (1.0 - a2) + a2);
    float GGXL = NoV * sqrt(NoL * NoL * (1.0 - a2) + a2);
    return 0.5 / max(GGXV + GGXL, 1e-6);
}

// Fast approximation - Hammon 2017
float V_SmithGGXCorrelated_Fast(float NoV, float NoL, float roughness) {
    return 0.5 / mix(2.0 * NoL * NoV, NoL + NoV, roughness);
}

// Kelemen visibility for clear coat - Kelemen 2001
float V_Kelemen(float LoH) {
    return 0.25 / max(LoH * LoH, 1e-6);
}

// Neubelt visibility for cloth - Neubelt and Pettineo 2013
float V_Neubelt(float NoV, float NoL) {
    return 1.0 / max(4.0 * (NoL + NoV - NoL * NoV), 1e-6);
}

// Anisotropic Smith visibility
float V_SmithGGXCorrelated_Anisotropic(float at, float ab, float ToV, float BoV, float ToL, float BoL, float NoV, float NoL) {
    float lambdaV = NoL * length(vec3(at * ToV, ab * BoV, NoV));
    float lambdaL = NoV * length(vec3(at * ToL, ab * BoL, NoL));
    return 0.5 / max(lambdaV + lambdaL, 1e-6);
}

// -----------------------------------------------------------------------------
// Specular F - Schlick Fresnel Approximation
// -----------------------------------------------------------------------------
// Approximates the Fresnel reflectance at the microfacet level.
// f0 = reflectance at normal incidence
// f90 = reflectance at grazing angle (usually 1.0 for smooth materials)

vec3 F_Schlick(float VoH, vec3 f0, float f90) {
    float fc = pow(1.0 - VoH, 5.0);
    return f0 + (vec3(f90) - f0) * fc;
}

// Simplified version assuming f90 = 1.0 (common for smooth materials)
vec3 F_Schlick(float VoH, vec3 f0) {
    float f = pow(1.0 - VoH, 5.0);
    return f + f0 * (1.0 - f);
}

// Scalar version for single-channel Fresnel
float F_Schlick(float VoH, float f0, float f90) {
    float fc = pow(1.0 - VoH, 5.0);
    return f0 + (f90 - f0) * fc;
}

// -----------------------------------------------------------------------------
// Diffuse BRDF - Lambertian
// -----------------------------------------------------------------------------
// Simple Lambertian diffuse assuming uniform diffuse response.
// The 1/PI normalization ensures energy conservation.

float Fd_Lambert() {
    return 1.0 / PI;
}

// Disney/Burley diffuse - Burley 2012
// More accurate retro-reflection for rough surfaces
float Fd_Burley(float NoV, float NoL, float LoH, float roughness) {
    float f90 = 0.5 + 2.0 * roughness * LoH * LoH;
    float lightScatter = F_Schlick(NoL, 1.0, f90);
    float viewScatter = F_Schlick(NoV, 1.0, f90);
    return lightScatter * viewScatter * (1.0 / PI);
}

// -----------------------------------------------------------------------------
// Combined BRDF Evaluation for Direct Lighting
// -----------------------------------------------------------------------------
// Evaluates the full Cook-Torrance specular + Lambertian diffuse BRDF.
// Returns the combined surface response for a single light.

struct SurfaceData {
    vec3 diffuseColor;   // Base color modified for metallic
    vec3 f0;             // Specular reflectance at normal incidence
    float roughness;     // Perceptual roughness squared and clamped
    float f90;           // Reflectance at grazing angle (usually 1.0)
};

struct LightVectors {
    float NoV;  // Normal dot View
    float NoL;  // Normal dot Light
    float NoH;  // Normal dot Half
    float LoH;  // Light dot Half (same as VoH)
    float VoH;  // View dot Half
};

// Compute all lighting vectors from normal, view, and light directions
LightVectors computeLightVectors(vec3 n, vec3 v, vec3 l) {
    vec3 h = normalize(v + l);
    
    LightVectors lv;
    lv.NoV = abs(dot(n, v)) + MIN_N_DOT_V;  // Avoid artifacts for back-facing
    lv.NoL = saturate(dot(n, l));
    lv.NoH = saturate(dot(n, h));
    lv.LoH = saturate(dot(l, h));
    lv.VoH = saturate(dot(v, h));
    return lv;
}

// Evaluate the specular BRDF (Cook-Torrance microfacet model)
vec3 specularBRDF(SurfaceData surface, LightVectors lv) {
    float D = D_GGX(lv.NoH, surface.roughness);
    float V = V_SmithGGXCorrelated(lv.NoV, lv.NoL, surface.roughness);
    vec3 F = F_Schlick(lv.VoH, surface.f0, surface.f90);
    return (D * V) * F;
}

// Evaluate the diffuse BRDF (Lambertian)
vec3 diffuseBRDF(SurfaceData surface) {
    return surface.diffuseColor * Fd_Lambert();
}

// Full surface shading for a single light (diffuse + specular)
vec3 surfaceShading(SurfaceData surface, LightVectors lv) {
    vec3 Fr = specularBRDF(surface, lv);  // Specular reflection
    vec3 Fd = diffuseBRDF(surface);       // Diffuse reflection
    
    // Energy conservation: specular energy removes from diffuse
    // This is approximated by the Fresnel term at normal incidence
    // More accurate would use (1 - F) but F depends on VoH, not a constant
    // For now, we use the simple additive model (Filament does similar)
    return Fd + Fr;
}

// -----------------------------------------------------------------------------
// Ambient Occlusion Functions
// -----------------------------------------------------------------------------

// Multi-bounce AO approximation from Filament
// This accounts for inter-reflections inside cavities, preserving color
// Without this, shadowed areas appear too dark and desaturated
vec3 multiBounceAO(float visibility, vec3 albedo) {
    // Inter-reflection approximation
    vec3 a = 2.0404 * albedo - 0.3324;
    vec3 b = -4.7951 * albedo + 0.6417;
    vec3 c = 2.7552 * albedo + 0.6903;
    float x = visibility;
    return max(vec3(x), ((x * a + b) * x + c) * x);
}

// Specular occlusion from AO - reduces specular in occluded areas
// Based on "Practical Realtime Strategies for Accurate Indirect Occlusion"
float specularAO(float NoV, float visibility, float roughness) {
    return saturate(pow(NoV + visibility, exp2(-16.0 * roughness - 1.0)) - 1.0 + visibility);
}

// Micro-shadowing - Chan 2018, "Material Advances in Call of Duty: WWII"
// Darkens direct lighting in occluded areas based on AO
float computeMicroShadowing(float NoL, float visibility) {
    float aperture = inversesqrt(1.0 - min(visibility, 0.9999));
    float microShadow = saturate(NoL * aperture);
    return microShadow * microShadow;
}

// Curvature-based ambient occlusion (screen-space estimation)
// Uses normal derivatives to detect concave areas
float curvatureAO(vec3 normal, float radius, float strength) {
    vec3 dNdx = dFdx(normal);
    vec3 dNdy = dFdy(normal);
    float curvature = length(dNdx) + length(dNdy);
    float ao = 1.0 - clamp(curvature * radius * 2.0, 0.0, 0.6);
    return mix(1.0, ao, strength);
}

// Cavity/crevice detection using position derivatives
// Detects areas where geometry folds on itself
float cavityAO(vec3 normal, vec3 fragPos) {
    vec3 dPosdx = dFdx(fragPos);
    vec3 dPosdy = dFdy(fragPos);
    vec3 faceNormal = normalize(cross(dPosdx, dPosdy));
    float cavityFactor = dot(normal, faceNormal);
    return mix(0.75, 1.0, saturate(cavityFactor));
}

// Combined procedural AO (no texture required)
// Uses curvature and cavity detection from screen-space derivatives
float proceduralAO(vec3 normal, vec3 fragPos, float radius, float strength) {
    // Curvature-based AO from normal derivatives
    vec3 dNdx = dFdx(normal);
    vec3 dNdy = dFdy(normal);
    float curvature = length(dNdx) + length(dNdy);
    float curvAO = 1.0 - clamp(curvature * radius * 5.0, 0.0, 0.8);
    
    // Cavity detection from position derivatives
    vec3 dPosdx = dFdx(fragPos);
    vec3 dPosdy = dFdy(fragPos);
    vec3 faceNormal = normalize(cross(dPosdx, dPosdy));
    float cavity = mix(0.6, 1.0, saturate(dot(normal, faceNormal)));
    
    float ao = curvAO * cavity;
    ao = mix(1.0, ao, strength);
    return max(ao, 0.1);  // Minimum 10% to avoid completely black areas
}

// Apply AO to indirect lighting with multi-bounce correction
vec3 applyAO(vec3 indirectLight, vec3 diffuseColor, float ao) {
    // Apply multi-bounce to maintain color in shadows
    vec3 multiBounce = multiBounceAO(ao, diffuseColor);
    return indirectLight * multiBounce;
}

// Horizon-based AO falloff (for ground contact)
// Darkens areas near ground-facing surfaces
float horizonAO(vec3 normal, float intensity) {
    float horizon = saturate(1.0 - normal.y);
    horizon = pow(horizon, 2.0) * intensity;
    return 1.0 - horizon;
}

#endif // PBR_COMMON_GLSL

