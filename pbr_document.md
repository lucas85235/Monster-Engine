# Documentação Completa do Pipeline PBR - Filament Engine

> **Objetivo**: Guia ultra-detalhado para implementação de sistema PBR em engine própria baseado na análise do Google Filament

---

## PARTE 1: VISÃO GERAL E ARQUITETURA

### 1.1 Estrutura de Diretórios do Pipeline PBR

```
filament/
├── shaders/src/                    # Shaders GLSL do pipeline PBR
│   ├── surface_brdf.fs             # Funções BRDF (D, G, F)
│   ├── surface_shading_model_*.fs  # Modelos de shading (standard, cloth, subsurface)
│   ├── surface_lighting.fs         # Estruturas Light e PixelParams
│   ├── surface_shading_lit.fs      # Pipeline de iluminação principal
│   ├── surface_light_indirect.fs   # IBL (Image-Based Lighting)
│   ├── surface_light_directional.fs # Luz direcional (sol)
│   ├── surface_light_punctual.fs   # Luzes pontuais/spot (froxels)
│   ├── surface_shadowing.fs        # Shadow mapping (PCF, PCSS, VSM)
│   ├── surface_ambient_occlusion.fs # SSAO e specular AO
│   ├── surface_fog.fs              # Sistema de fog atmosférico
│   └── surface_material_inputs.fs  # Estrutura MaterialInputs
├── filament/src/
│   ├── PostProcessManager.cpp/h    # Bloom, blur, tone mapping
│   ├── ShadowMap.cpp/h             # Gerenciamento de shadow maps
│   └── Froxelizer.cpp/h            # Sistema de culling de luzes
└── libs/filamat/src/
    └── MaterialBuilder.cpp         # Compilador de materiais
```

### 1.2 Fluxo do Pipeline de Renderização

```
┌─────────────────────────────────────────────────────────────────┐
│                    MATERIAL INPUTS (User)                       │
│  baseColor, roughness, metallic, reflectance, normal, AO, etc.  │
└──────────────────────────┬──────────────────────────────────────┘
                           ▼
┌─────────────────────────────────────────────────────────────────┐
│                 getPixelParams() → PixelParams                  │
│  diffuseColor, f0, roughness, energyCompensation, dfg, etc.     │
└──────────────────────────┬──────────────────────────────────────┘
                           ▼
┌─────────────────────────────────────────────────────────────────┐
│                    evaluateLights()                             │
│  ┌─────────────┐  ┌─────────────────┐  ┌────────────────────┐   │
│  │ evaluateIBL │  │evaluateDirectional│  │evaluatePunctual │   │
│  └─────────────┘  └─────────────────┘  └────────────────────┘   │
└──────────────────────────┬──────────────────────────────────────┘
                           ▼
┌─────────────────────────────────────────────────────────────────┐
│                     POST-PROCESSING                             │
│  Fog → Bloom → Tone Mapping → Color Grading → Dithering         │
└─────────────────────────────────────────────────────────────────┘
```

### 1.3 Estruturas de Dados Fundamentais

#### MaterialInputs (Entrada do Usuário)
```glsl
struct MaterialInputs {
    vec4  baseColor;           // Cor base RGBA
    float roughness;           // Rugosidade [0-1]
    float metallic;            // Metalicidade [0-1]  
    float reflectance;         // Refletância dielétrica [0-1], default 0.5 (4% F0)
    float ambientOcclusion;    // AO [0-1]
    vec4  emissive;            // Emissão RGB + fator de atenuação exposição
    vec3  normal;              // Normal map (tangent space)
    
    // Opcionais por shading model
    float clearCoat;           // Intensidade clear coat [0-1]
    float clearCoatRoughness;  // Rugosidade clear coat
    float anisotropy;          // Anisotropia [-1, 1]
    vec3  anisotropyDirection; // Direção da anisotropia
    vec3  sheenColor;          // Cor do sheen
    float sheenRoughness;      // Rugosidade do sheen
    float subsurfacePower;     // Potência subsurface
    vec3  subsurfaceColor;     // Cor subsurface
    float thickness;           // Espessura para refração/subsurface
    float transmission;        // Transmissão [0-1]
    vec3  absorption;          // Coeficiente de absorção
    float ior;                 // Índice de refração (default 1.5)
};
```

#### PixelParams (Parâmetros Computados)
```glsl
struct PixelParams {
    vec3  diffuseColor;            // Cor difusa = baseColor * (1 - metallic)
    float perceptualRoughness;     // Rugosidade perceptual [0-1]
    vec3  f0;                      // Refletância em ângulo normal
    float roughness;               // Rugosidade linear (perceptual²)
    vec3  dfg;                     // DFG pré-filtrado do LUT
    vec3  energyCompensation;      // Compensação multi-scattering
    
    // Clear coat
    float clearCoat;
    float clearCoatRoughness;
    
    // Sheen
    vec3  sheenColor;
    float sheenRoughness;
    float sheenScaling;
    float sheenDFG;
    
    // Anisotropia
    vec3  anisotropicT;            // Tangente
    vec3  anisotropicB;            // Bitangente
    float anisotropy;
    
    // Refração
    float etaIR;                   // IOR ar → material
    float etaRI;                   // IOR material → ar
    float transmission;
    vec3  absorption;
    float thickness;
};
```

#### Light (Estrutura de Luz)
```glsl
struct Light {
    vec4 colorIntensity;      // RGB + intensidade pré-exposta
    vec3 l;                   // Direção da luz (normalizada)
    float attenuation;        // Atenuação por distância/ângulo
    highp vec3 worldPosition; // Posição mundo
    float NoL;                // dot(normal, light)
    highp vec3 direction;     // Direção do spotlight
    float zLight;             // Profundidade no light space
    bool castsShadows;        // Projeta sombras?
    bool contactShadows;      // Usa contact shadows?
    uint lightType;           // POINT = 0, SPOT = 1
    int shadowIndex;          // Índice no shadow map array
    int channels;             // Canais de luz (masking)
};
```

---

## PARTE 2: BRDF - BIDIRECTIONAL REFLECTANCE DISTRIBUTION FUNCTION

### 2.1 Equação de Renderização PBR

A equação fundamental do Filament:
```
Lo = ∫(kd * Fd + ks * Fr) * Li * (n·l) dω
```
Onde:
- **Fd**: BRDF difuso (Lambert ou Burley)
- **Fr**: BRDF especular (Cook-Torrance: D * G * F / (4 * NoV * NoL))
- **kd**: Coeficiente difuso = (1 - F) * (1 - metallic)
- **ks**: Coeficiente especular = F

### 2.2 Funções de Distribuição Normal (NDF) - Termo D

#### GGX/Trowbridge-Reitz (Padrão)
```glsl
// Walter et al. 2007, "Microfacet Models for Refraction through Rough Surfaces"
float D_GGX(float roughness, float NoH, const vec3 h) {
    // Lagrange's identity para evitar perda de precisão
    // ||N x H||² = 1 - NoH²
    float oneMinusNoHSquared = 1.0 - NoH * NoH;
    
    float a = NoH * roughness;
    float k = roughness / (oneMinusNoHSquared + a * a);
    float d = k * k * (1.0 / PI);
    return d;
}
```

#### GGX Anisotrópico
```glsl
// Burley 2012, "Physically-Based Shading at Disney"
float D_GGX_Anisotropic(float at, float ab, float ToH, float BoH, float NoH) {
    float a2 = at * ab;
    highp vec3 d = vec3(ab * ToH, at * BoH, a2 * NoH);
    highp float d2 = dot(d, d);
    float b2 = a2 / d2;
    return a2 * b2 * b2 * (1.0 / PI);
}
```

#### Charlie (para Sheen/Cloth)
```glsl
// Estevez and Kulla 2017, "Production Friendly Microfacet Sheen BRDF"
float D_Charlie(float roughness, float NoH) {
    float invAlpha = 1.0 / roughness;
    float cos2h = NoH * NoH;
    float sin2h = max(1.0 - cos2h, 0.0078125);
    return (2.0 + invAlpha) * pow(sin2h, invAlpha * 0.5) / (2.0 * PI);
}
```

### 2.3 Funções de Geometria/Visibilidade - Termo G (V = G / (4*NoV*NoL))

#### Smith GGX Height-Correlated
```glsl
// Heitz 2014, "Understanding the Masking-Shadowing Function"
float V_SmithGGXCorrelated(float roughness, float NoV, float NoL) {
    float a2 = roughness * roughness;
    float lambdaV = NoL * sqrt((NoV - a2 * NoV) * NoV + a2);
    float lambdaL = NoV * sqrt((NoL - a2 * NoL) * NoL + a2);
    float v = 0.5 / (lambdaV + lambdaL);
    return v;
}
```

#### Smith GGX Fast (Mobile)
```glsl
// Hammon 2017, "PBR Diffuse Lighting for GGX+Smith Microsurfaces"
float V_SmithGGXCorrelated_Fast(float roughness, float NoV, float NoL) {
    float v = 0.5 / mix(2.0 * NoL * NoV, NoL + NoV, roughness);
    return v;
}
```

#### Kelemen (Clear Coat)
```glsl
// Kelemen 2001, "A Microfacet Based Coupled Specular-Matte BRDF Model"
float V_Kelemen(float LoH) {
    return 0.25 / (LoH * LoH);
}
```

#### Neubelt (Cloth)
```glsl
// Neubelt and Pettineo 2013, "Crafting a Next-gen Material Pipeline for The Order: 1886"
float V_Neubelt(float NoV, float NoL) {
    return 1.0 / (4.0 * (NoL + NoV - NoL * NoV));
}
```

### 2.4 Função de Fresnel - Termo F

#### Schlick Approximation
```glsl
// Schlick 1994, "An Inexpensive BRDF Model for Physically-Based Rendering"
vec3 F_Schlick(const vec3 f0, float f90, float VoH) {
    return f0 + (f90 - f0) * pow(1.0 - VoH, 5.0);
}

// Versão simplificada
vec3 F_Schlick(const vec3 f0, float VoH) {
    float f = pow(1.0 - VoH, 5.0);
    return f + f0 * (1.0 - f);
}
```

### 2.5 BRDF Difuso

#### Lambert (Padrão - mais eficiente)
```glsl
float Fd_Lambert() {
    return 1.0 / PI;
}
```

#### Burley (Disney - mais preciso)
```glsl
// Burley 2012, "Physically-Based Shading at Disney"
float Fd_Burley(float roughness, float NoV, float NoL, float LoH) {
    float f90 = 0.5 + 2.0 * roughness * LoH * LoH;
    float lightScatter = F_Schlick(1.0, f90, NoL);
    float viewScatter  = F_Schlick(1.0, f90, NoV);
    return lightScatter * viewScatter * (1.0 / PI);
}
```

### 2.6 Cálculo de F0 (Refletância em Ângulo Normal)

```glsl
// Para dielétricos: baseado em reflectance [0-1]
float computeDielectricF0(float reflectance) {
    return 0.16 * reflectance * reflectance;  // 4% para reflectance=0.5
}

// F0 final misturando dielétrico e metálico
vec3 computeF0(vec4 baseColor, float metallic, float reflectance) {
    vec3 dielectricF0 = vec3(computeDielectricF0(reflectance));
    return mix(dielectricF0, baseColor.rgb, metallic);
}

// Cor difusa (metais não têm difuso)
vec3 computeDiffuseColor(vec4 baseColor, float metallic) {
    return baseColor.rgb * (1.0 - metallic);
}
```

---

---

## PARTE 3: SHADING MODELS

### 3.1 Standard Shading Model (Padrão)

O modelo padrão combina BRDF difuso + especular com suporte a clear coat e anisotropia:

```glsl
vec3 surfaceShading(const PixelParams pixel, const Light light, float occlusion) {
    vec3 h = normalize(shading_view + light.l);  // half-vector

    float NoV = shading_NoV;
    float NoL = saturate(light.NoL);
    float NoH = saturate(dot(shading_normal, h));
    float LoH = saturate(dot(light.l, h));

    // Lobe especular (isotropic ou anisotropic)
    vec3 Fr = specularLobe(pixel, light, h, NoV, NoL, NoH, LoH);
    
    // Lobe difuso
    vec3 Fd = diffuseLobe(pixel, NoV, NoL, LoH);
    
    // Compensação de energia para múltiplo espalhamento
    vec3 color = Fd + Fr * pixel.energyCompensation;

    // Sheen layer (opcional)
    #if defined(MATERIAL_HAS_SHEEN_COLOR)
    color *= pixel.sheenScaling;
    color += sheenLobe(pixel, NoV, NoL, NoH);
    #endif

    // Clear Coat layer (opcional)
    #if defined(MATERIAL_HAS_CLEAR_COAT)
    float Fcc;
    float clearCoat = clearCoatLobe(pixel, h, NoH, LoH, Fcc);
    float attenuation = 1.0 - Fcc;
    color *= attenuation;
    color += clearCoat;
    #endif

    return (color * light.colorIntensity.rgb) *
            (light.colorIntensity.w * light.attenuation * NoL * occlusion);
}
```

#### Lobe Especular Isotrópico
```glsl
vec3 isotropicLobe(const PixelParams pixel, const Light light, const vec3 h,
        float NoV, float NoL, float NoH, float LoH) {
    float D = distribution(pixel.roughness, NoH, h);   // GGX
    float V = visibility(pixel.roughness, NoV, NoL);   // Smith
    vec3  F = fresnel(pixel.f0, LoH);                  // Schlick
    return (D * V) * F;
}
```

#### Clear Coat Lobe
```glsl
float clearCoatLobe(const PixelParams pixel, const vec3 h, float NoH, float LoH, out float Fcc) {
    // Clear coat usa IOR fixo de 1.5 (4% refletância)
    float D = distributionClearCoat(pixel.clearCoatRoughness, NoH, h);
    float V = visibilityClearCoat(LoH);  // Kelemen
    float F = F_Schlick(0.04, 1.0, LoH) * pixel.clearCoat;
    
    Fcc = F;
    return D * V * F;
}
```

### 3.2 Cloth Shading Model

Para tecidos, usa distribuição Charlie e visibilidade Neubelt:

```glsl
vec3 surfaceShading(const PixelParams pixel, const Light light, float occlusion) {
    vec3 h = normalize(shading_view + light.l);
    float NoL = saturate(light.NoL);
    float NoH = saturate(dot(shading_normal, h));

    // Specular BRDF usando Charlie NDF
    float D = distributionCloth(pixel.roughness, NoH);  // D_Charlie
    float V = visibilityCloth(shading_NoV, NoL);        // V_Neubelt
    vec3  F = pixel.sheenColor;
    vec3 Fr = (D * V) * F;

    // Diffuse BRDF
    float diffuse = diffuse(pixel.roughness, shading_NoV, NoL, LoH);
    vec3 Fd = diffuse * pixel.diffuseColor;

    // Subsurface scattering (opcional)
    #if defined(MATERIAL_HAS_SUBSURFACE_COLOR)
    Fd *= saturate(pixel.subsurfaceColor + shading_NoV);
    #endif

    vec3 color = Fd + Fr * pixel.energyCompensation;
    return (color * light.colorIntensity.rgb) *
            (light.colorIntensity.w * light.attenuation * NoL * occlusion);
}
```

### 3.3 Subsurface Shading Model

Para materiais translúcidos (pele, cera, folhas):

```glsl
vec3 surfaceShading(const PixelParams pixel, const Light light, float occlusion) {
    vec3 h = normalize(shading_view + light.l);
    
    float NoV = shading_NoV;
    float NoL = saturate(light.NoL);
    float NoH = saturate(dot(shading_normal, h));
    float LoH = saturate(dot(light.l, h));

    vec3 Fr = specularLobe(pixel, light, h, NoV, NoL, NoH, LoH);
    vec3 Fd = diffuseLobe(pixel, NoV, NoL, LoH);

    // Subsurface scattering usando wrap lighting
    float scatterVoH = saturate(dot(shading_view, -light.l));
    float forwardScatter = exp2(scatterVoH * pixel.subsurfacePower - pixel.subsurfacePower);
    float backScatter = saturate(NoL * pixel.thickness + (1.0 - pixel.thickness)) * 0.5;
    float subsurface = mix(backScatter, 1.0, forwardScatter) * (1.0 - pixel.thickness);
    
    vec3 color = Fd + Fr;
    color += pixel.subsurfaceColor * (subsurface * Fd_Lambert());

    return (color * light.colorIntensity.rgb) *
            (light.colorIntensity.w * light.attenuation * occlusion);
}
```

### 3.4 Compensação de Energia (Multi-Scattering)

O Filament compensa a perda de energia em superfícies rugosas:

```glsl
void getEnergyCompensationPixelParams(inout PixelParams pixel) {
    // DFG pré-filtrado do LUT
    pixel.dfg = prefilteredDFG(pixel.perceptualRoughness, shading_NoV);
    
    // Compensação multi-scattering
    // Kulla and Conty 2017, "Revisiting Physically Based Shading at Imageworks"
    pixel.energyCompensation = 1.0 + pixel.f0 * (1.0 / pixel.dfg.y - 1.0);
}
```

---

## PARTE 4: IMAGE-BASED LIGHTING (IBL)

### 4.1 Visão Geral do IBL

O IBL fornece iluminação ambiente usando:
1. **Irradiance** (difuso): Spherical Harmonics ou cubemap pré-filtrado
2. **Radiance** (especular): Cubemap pré-filtrado com mipmaps por roughness
3. **DFG LUT**: Lookup table 2D para BRDF pré-integrado

### 4.2 Irradiância Difusa via Spherical Harmonics

```glsl
vec3 Irradiance_SphericalHarmonics(const vec3 n) {
    vec3 sh = frameUniforms.iblSH[0];  // L00

    // Band 1 (L1-1, L10, L11)
    sh += frameUniforms.iblSH[1] * n.y;
    sh += frameUniforms.iblSH[2] * n.z;
    sh += frameUniforms.iblSH[3] * n.x;

    // Band 2 (5 coeficientes)
    sh += frameUniforms.iblSH[4] * (n.y * n.x);
    sh += frameUniforms.iblSH[5] * (n.y * n.z);
    sh += frameUniforms.iblSH[6] * (3.0 * n.z * n.z - 1.0);
    sh += frameUniforms.iblSH[7] * (n.z * n.x);
    sh += frameUniforms.iblSH[8] * (n.x * n.x - n.y * n.y);

    return max(sh, 0.0);
}
```

### 4.3 Radiância Especular via Cubemap Pré-filtrado

```glsl
float perceptualRoughnessToLod(float perceptualRoughness) {
    // Mapeamento quadrático para LOD
    return iblRoughnessOneLevel * perceptualRoughness * (2.0 - perceptualRoughness);
}

vec3 prefilteredRadiance(const vec3 r, float perceptualRoughness) {
    float lod = perceptualRoughnessToLod(perceptualRoughness);
    return textureLod(iblSpecular, r, lod).rgb;
}
```

### 4.4 DFG Lookup Table

```glsl
vec3 PrefilteredDFG_LUT(float lod, float NoV) {
    // x = sqrt(roughness) (perceptual roughness)
    // y = NoV
    return textureLod(iblDFG, vec2(NoV, lod), 0.0).rgb;
}

// Uso no IBL specular
vec3 E = specularDFG(pixel);  // mix(dfg.xxx, dfg.yyy, f0)
vec3 Fr = E * prefilteredRadiance(r, pixel.perceptualRoughness);
```

### 4.5 Pipeline IBL Completo

```glsl
void evaluateIBL(const MaterialInputs material, const PixelParams pixel, inout vec3 color) {
    // 1. Specular IBL
    vec3 E = specularDFG(pixel);
    vec3 r = getReflectedVector(pixel, shading_normal);
    vec3 Fr = E * prefilteredRadiance(r, pixel.perceptualRoughness);

    // 2. Ambient Occlusion
    float ssao = evaluateSSAO();
    float diffuseAO = min(material.ambientOcclusion, ssao);
    float specularAO = specularAO(shading_NoV, diffuseAO, pixel.roughness);
    
    Fr *= singleBounceAO(specularAO) * pixel.energyCompensation;

    // 3. Diffuse IBL
    float diffuseBRDF = singleBounceAO(diffuseAO);
    vec3 diffuseIrradiance = diffuseIrradiance(shading_normal);  // SH
    vec3 Fd = pixel.diffuseColor * diffuseIrradiance * (1.0 - E) * diffuseBRDF;

    // 4. Subsurface/Cloth
    evaluateSubsurfaceIBL(pixel, diffuseIrradiance, Fd, Fr);

    // 5. Multi-bounce AO
    multiBounceAO(diffuseAO, pixel.diffuseColor, Fd);
    multiBounceSpecularAO(specularAO, pixel.f0, Fr);

    // 6. Sheen + Clear Coat
    evaluateSheenIBL(pixel, diffuseAO, Fd, Fr);
    evaluateClearCoatIBL(pixel, diffuseAO, Fd, Fr);

    // 7. IBL luminance
    Fr *= frameUniforms.iblLuminance;
    Fd *= frameUniforms.iblLuminance;

    color.rgb += Fr + Fd;
}
```

### 4.6 Screen Space Reflections (SSR)

```glsl
// SSR integrado com IBL
vec4 ssrFr = vec4(0.0);
if (frameUniforms.ssrDistance > 0.0 && pixel.perceptualRoughness < 0.707) {
    float lod = log2(pixel.roughness / distance) + refractionLodOffset;
    ssrFr = textureLod(ssrTexture, vec3(uv, 1.0), lod);
}

// Blend SSR com IBL
Fr = Fr * (1.0 - ssrFr.a) + (E * ssrFr.rgb);
```

## PARTE 5: SHADOW MAPPING

### 5.1 Técnicas de Shadow Sampling Disponíveis

| Técnica | Qualidade | Performance | Uso |
|---------|-----------|------------|-----|
| PCF Hard | Baixa | Muito Alta | Mobile low-end |
| PCF Low (3x3) | Média | Alta | Mobile padrão |
| DPCF | Alta | Média | Desktop/Console |
| PCSS | Muito Alta | Baixa | Cinematics |
| VSM/EVSM | Alta | Média | Sombras suaves |

### 5.2 PCF (Percentage Closer Filtering)

#### PCF Hard (1 sample)
```glsl
float ShadowSample_PCF_Hard(sampler2DArrayShadow map, uint layer, vec4 shadowPosition) {
    vec3 position = shadowPosition.xyz / shadowPosition.w;
    return texture(map, vec4(position.xy, layer, saturate(position.z)));
}
```

#### PCF 3x3 Gaussian
```glsl
float ShadowSample_PCF_Low(sampler2DArrayShadow map, uint layer, vec4 shadowPosition) {
    vec3 position = shadowPosition.xyz / shadowPosition.w;
    vec2 size = vec2(textureSize(map, 0));
    vec2 texelSize = 1.0 / size;

    // Castaño 2013, "Shadow Mapping Summary Part 1"
    vec2 uv = position.xy * size + 0.5;
    vec2 base = (floor(uv) - 0.5) * texelSize;
    vec2 st = fract(uv);

    // Pesos bilineares 3x3
    vec2 uw = vec2(3.0 - 2.0 * st.x, 1.0 + 2.0 * st.x);
    vec2 vw = vec2(3.0 - 2.0 * st.y, 1.0 + 2.0 * st.y);

    vec2 u = vec2((2.0 - st.x) / uw.x - 1.0, st.x / uw.y + 1.0) * texelSize.x;
    vec2 v = vec2((2.0 - st.y) / vw.x - 1.0, st.y / vw.y + 1.0) * texelSize.y;

    float sum = 0.0;
    sum += uw.x * vw.x * sampleDepth(map, layer, base + vec2(u.x, v.x), position.z);
    sum += uw.y * vw.x * sampleDepth(map, layer, base + vec2(u.y, v.x), position.z);
    sum += uw.x * vw.y * sampleDepth(map, layer, base + vec2(u.x, v.y), position.z);
    sum += uw.y * vw.y * sampleDepth(map, layer, base + vec2(u.y, v.y), position.z);
    return sum / 16.0;
}
```

### 5.3 DPCF (Distance-based PCF) - Contact Hardening

```glsl
// Myers, "Shadow of Cold War" - Scalable approach to shadowing
float ShadowSample_DPCF(sampler2DArray map, uint layer, int index, 
        vec4 shadowPosition, float zLight) {
    vec3 position = shadowPosition.xyz / shadowPosition.w;
    vec2 texelSize = 1.0 / vec2(textureSize(map, 0));

    // Receiver plane depth bias
    vec2 dz_duv = computeReceiverPlaneDepthBias(position);
    float penumbra = shadowUniforms.shadows[index].bulbRadiusLs;
    
    mat2 R = getRandomRotationMatrix(gl_FragCoord.xy);

    // Blocker search
    float occludedCount = 0.0;
    float z_occSum = 0.0;
    for (uint i = 0u; i < DPCF_SHADOW_TAP_COUNT; i++) {
        vec2 duv = R * (poissonDisk[i] * texelSize * penumbra);
        float z_occ = textureLod(map, vec3(position.xy + duv, layer), 0.0).r;
        float z_bias = dot(dz_duv, duv);
        float occluded = step(z_bias, z_occ - position.z);
        occludedCount += occluded;
        z_occSum += z_occ * occluded;
    }

    if (z_occSum == 0.0) return 1.0;

    // Penumbra ratio
    float penumbraRatio = saturate(getPenumbraRatio(position.z, z_occSum / occludedCount));
    
    // Blend between hard and soft kernel
    float percentageOccluded = occludedCount / float(DPCF_SHADOW_TAP_COUNT);
    percentageOccluded = mix(hardenedKernel(percentageOccluded), percentageOccluded, penumbraRatio);
    return 1.0 - percentageOccluded;
}
```

### 5.4 VSM (Variance Shadow Maps)

```glsl
float evaluateShadowVSM(vec2 moments, float depth) {
    // Donnelly and Lauritzen 2006, "Variance Shadow Maps"
    float variance = moments.y - (moments.x * moments.x);
    variance = max(variance, minVariance);

    float d = depth - moments.x;
    float pMax = variance / (variance + d * d);
    
    // Reduce light bleeding
    pMax = linstep(lightBleedReduction, 1.0, pMax);
    
    return depth <= moments.x ? 1.0 : pMax;
}

float ShadowSample_VSM(sampler2DArray shadowMap, uint layer, vec4 shadowPosition) {
    vec3 position = vec3(shadowPosition.xy / shadowPosition.w, shadowPosition.z);
    vec4 moments = texture(shadowMap, vec3(position.xy, layer));
    
    // EVSM depth warping
    float depth = exp(vsmExponent * (position.z * 2.0 - 1.0));
    
    float p = evaluateShadowVSM(moments.xy, depth);
    // ELVSM: também avalia o momento negativo
    p = min(p, evaluateShadowVSM(moments.zw, -1.0 / depth));
    return p;
}
```

### 5.5 Contact Shadows (Screen-Space)

```glsl
float screenSpaceContactShadow(vec3 lightDirection) {
    ScreenSpaceRay ray;
    initScreenSpaceRay(ray, shading_position, lightDirection, kDistanceMax);

    float dt = 1.0 / float(kStepCount);
    float tolerance = abs(ray.ssViewRayEnd.z - ray.ssRayStart.z) * dt;
    float dither = interleavedGradientNoise(gl_FragCoord.xy) - 0.5;
    float t = dt * dither + dt;

    for (int i = 0; i < kStepCount; i++, t += dt) {
        vec3 ray = ray.uvRayStart + ray.uvRay * t;
        float z = textureLod(depthTexture, ray.xy, 0.0).r;
        float dz = z - ray.z;
        if (abs(tolerance - dz) < tolerance) {
            return 1.0;  // Ocluído
        }
    }
    return 0.0;
}
```

### 5.6 Cascaded Shadow Maps (Luz Direcional)

```glsl
int getShadowCascade() {
    // Determina qual cascade usar baseado na distância da câmera
    vec4 z = cycleVec4(getViewFromWorldMatrix() * vec4(shading_position, 1.0));
    bvec4 greaterZ = greaterThan(z, frameUniforms.cascadeSplits);
    return cycleVec4(greaterZ) ? clamp(findMSB(uint(greaterZ)), 0, cascadeCount - 1) : 0;
}

void evaluateDirectionalLight(const MaterialInputs material, const PixelParams pixel, inout vec3 color) {
    Light light = getDirectionalLight();
    
    float visibility = 1.0;
    if (light.NoL > 0.0) {
        int cascade = getShadowCascade();
        if (cascadeHasVisibleShadows(cascade)) {
            vec4 shadowPosition = getShadowPosition(cascade);
            visibility = shadow(true, shadowMap, cascade, shadowPosition, 0.0);
        }
        
        // Contact shadows
        if (contactShadowsEnabled && visibility > 0.0) {
            visibility *= 1.0 - screenSpaceContactShadow(light.l);
        }
        
        // Micro-shadowing from AO
        visibility *= computeMicroShadowing(light.NoL, material.ambientOcclusion);
    }

    color.rgb += surfaceShading(pixel, light, visibility);
}
```

---

## PARTE 6: FOG ATMOSFÉRICO

### 6.1 Fog Exponencial com Altura

```glsl
// Wenzel, "Real-time Atmospheric Effects in Games"
vec4 fog(vec4 color, vec3 view) {
    float d = length(view);
    
    if (d < fogStart || d > fogCutOffDistance) return color;

    // Densidade com falloff de altura
    // density.x = densidade base
    // density.y = -falloff * height
    // density.z = density * exp(-falloff * height)
    vec3 density = frameUniforms.fogDensity;
    float falloff = frameUniforms.fogHeightFalloff;

    // Optical path para fog com altura variável
    float fogOpticalPathAtOneMeter = density.z;
    float fh = falloff * view.y;
    if (abs(fh) > 0.00125) {
        fogOpticalPathAtOneMeter = (density.z - density.x * exp(density.y - fh)) / fh;
    }

    // Beer-Lambert Law
    float fogOpticalPath = fogOpticalPathAtOneMeter * max(d - fogStart, 0.0);
    float fogTransmittance = exp(-fogOpticalPath);
    float fogOpacity = min(1.0 - fogTransmittance, fogMaxOpacity);

    // Cor do fog (pode vir do IBL)
    vec3 fogColor = frameUniforms.fogColor;
    if (fogColorFromIbl > 0.0) {
        float lod = mix(minMip, maxMip, saturate(normalizedDepth));
        fogColor *= textureLod(iblSpecular, view, lod).rgb;
    }
    fogColor *= iblLuminance * fogOpacity;

    // Inscattering do sol
    if (fogInscatteringSize > 0.0) {
        float sunOpticalPath = fogOpticalPathAtOneMeter * max(d - fogInscatteringStart, 0.0);
        float sunTransmittance = exp(-sunOpticalPath);
        
        vec3 sunColor = lightColorIntensity.rgb * lightColorIntensity.w;
        float sunAmount = max(dot(normalize(view), lightDirection), 0.0);
        float sunInscattering = pow(sunAmount, fogInscatteringSize);
        
        fogColor += sunColor * (sunInscattering * (1.0 - sunTransmittance));
    }

    color.rgb = color.rgb * (1.0 - fogOpacity) + fogColor;
    return color;
}
```

### 6.2 Fog Linear (Simplificado)

```glsl
vec4 fogLinear(vec4 color, vec3 view) {
    float d = length(view);
    if (d < fogStart || d > fogCutOffDistance) return color;

    float fogOpacity = saturate(A * d + B);
    vec3 fogColor = frameUniforms.fogColor * iblLuminance * fogOpacity;

    color.rgb = color.rgb * (1.0 - fogOpacity) + fogColor;
    return color;
}
```

## PARTE 7: AMBIENT OCCLUSION

### 7.1 SSAO com Bilateral Upscale

```glsl
float evaluateSSAO(inout SSAOInterpolationCache cache) {
    if (aoSamplingQualityAndEdgeDistance < 0.0) return 1.0;  // Desabilitado

    if (aoSamplingQualityAndEdgeDistance > 0.0) {
        // High quality bilateral upscale
        vec4 ao = textureGather(ssaoTexture, cache.uv, 0);
        vec4 dg = textureGather(ssaoTexture, cache.uv, 1);  // Depth green
        vec4 db = textureGather(ssaoTexture, cache.uv, 2);  // Depth blue

        // Unpack depths
        vec4 depths;
        depths.x = unpack(vec2(dg.x, db.x));
        depths.y = unpack(vec2(dg.y, db.y));
        depths.z = unpack(vec2(dg.z, db.z));
        depths.w = unpack(vec2(dg.w, db.w));
        depths *= -cameraFar;

        // Bilinear weights
        vec2 f = fract(cache.uv * size - 0.5);
        vec4 b;
        b.x = (1.0 - f.x) * f.y;
        b.y = f.x * f.y;
        b.z = f.x * (1.0 - f.y);
        b.w = (1.0 - f.x) * (1.0 - f.y);

        // Bilateral weights baseados na profundidade
        float d = dot(viewMatrix[2].xyz, shading_position) + viewMatrix[3].z;
        vec4 w = max(0.0, 1.0 - sq(vec4(d) - depths) * edgeDistance) * b;
        cache.weights = w / (w.x + w.y + w.z + w.w);
        
        return dot(ao, cache.weights);
    }
    return textureLod(ssaoTexture, cache.uv, 0.0).r;
}
```

### 7.2 Specular AO (Lagarde - Frostbite)

```glsl
// Lagarde and de Rousiers 2014, "Moving Frostbite to PBR"
float SpecularAO_Lagarde(float NoV, float visibility, float roughness) {
    return saturate(pow(NoV + visibility, exp2(-16.0 * roughness - 1.0)) - 1.0 + visibility);
}
```

### 7.3 Specular AO com Bent Normals (Cones)

```glsl
// Jimenez et al. 2016, "Practical Realtime Strategies for Accurate Indirect Occlusion"
float SpecularAO_Cones(vec3 bentNormal, float visibility, float roughness) {
    // Aperture from AO
    float cosAv = sqrt(1.0 - visibility);
    // Aperture from roughness
    float cosAs = exp2(-3.321928 * sq(roughness));
    // Angle between bent normal and reflection
    float cosB = dot(bentNormal, shading_reflected);

    float ao = sphericalCapsIntersection(cosAv, cosAs, cosB) / (1.0 - cosAs);
    
    // Smoothly kill specular AO for low roughness (metals)
    return mix(1.0, ao, smoothstep(0.01, 0.09, roughness));
}
```

### 7.4 Multi-Bounce AO

```glsl
// Jimenez et al. 2016 - Recupera energia perdida pelo AO
vec3 gtaoMultiBounce(float visibility, const vec3 albedo) {
    vec3 a =  2.0404 * albedo - 0.3324;
    vec3 b = -4.7951 * albedo + 0.6417;
    vec3 c =  2.7552 * albedo + 0.6903;
    return max(vec3(visibility), ((visibility * a + b) * visibility + c) * visibility);
}

void multiBounceAO(float visibility, const vec3 albedo, inout vec3 color) {
    color *= gtaoMultiBounce(visibility, albedo);
}
```

### 7.5 Micro-Shadowing

```glsl
// Chan 2018, "Material Advances in Call of Duty: WWII"
float computeMicroShadowing(float NoL, float visibility) {
    float aperture = inversesqrt(1.0 - min(visibility, 0.9999));
    float microShadow = saturate(NoL * aperture);
    return microShadow * microShadow;
}
```

---

## PARTE 8: SISTEMA DE LUZES

### 8.1 Froxel-Based Light Culling

O Filament usa Froxels (frustum + voxels) para culling eficiente de luzes:

```glsl
uvec3 getFroxelCoords(const vec3 fragCoords) {
    uvec3 froxelCoord;
    froxelCoord.xy = uvec2(fragCoords.xy * froxelCountXY);
    
    // Log-space Z slicing
    float viewSpaceZ = zParams.x * fragCoords.z + zParams.y;
    float sliceZ = log2(viewSpaceZ) * zParams.z + zSliceCount;
    froxelCoord.z = uint(clamp(sliceZ, 0.0, zSliceCount - 1.0));
    
    return froxelCoord;
}

FroxelParams getFroxelParams(uint froxelIndex) {
    uint data = froxelsUniforms.records[froxelIndex >> 2][froxelIndex & 3];
    FroxelParams froxel;
    froxel.recordOffset = data >> 16u;
    froxel.count = data & 0xFFu;
    return froxel;
}
```

### 8.2 Atenuação de Luzes

```glsl
// Square falloff (physically correct)
float getSquareFalloffAttenuation(float distanceSquare, float falloff) {
    float factor = distanceSquare * falloff;
    float smoothFactor = saturate(1.0 - factor * factor);
    return smoothFactor * smoothFactor;
}

float getDistanceAttenuation(const vec3 posToLight, float falloff) {
    float distanceSquare = dot(posToLight, posToLight);
    float attenuation = getSquareFalloffAttenuation(distanceSquare, falloff);
    
    // Light far attenuation
    vec3 v = worldPosition - cameraPosition;
    attenuation *= saturate(lightFarAttenuationParams.x - dot(v, v) * lightFarAttenuationParams.y);
    
    return attenuation / max(distanceSquare, 1e-4);
}

// Spotlight cone attenuation
float getAngleAttenuation(const vec3 lightDir, const vec3 l, const vec2 scaleOffset) {
    float cd = dot(lightDir, l);
    float attenuation = saturate(cd * scaleOffset.x + scaleOffset.y);
    return attenuation * attenuation;
}
```

### 8.3 Avaliação de Luzes Punctuais

```glsl
void evaluatePunctualLights(const MaterialInputs material, const PixelParams pixel, inout vec3 color) {
    FroxelParams froxel = getFroxelParams(getFroxelIndex(fragCoord));
    int channels = object_uniforms_flagsChannels & 0xFF;

    for (uint index = froxel.recordOffset; index < froxel.recordOffset + froxel.count; index++) {
        uint lightIndex = getLightIndex(index);
        Light light = getLight(lightIndex);
        
        if ((light.channels & channels) == 0) continue;
        if (light.NoL <= 0.0 || light.attenuation <= 0.0) continue;

        float visibility = 1.0;
        if (light.castsShadows) {
            vec4 shadowPosition = getShadowPosition(light.shadowIndex, light.direction, light.zLight);
            visibility = shadow(false, shadowMap, light.shadowIndex, shadowPosition, light.zLight);
        }
        if (light.contactShadows && visibility > 0.0) {
            visibility *= 1.0 - screenSpaceContactShadow(light.l);
        }

        color.rgb += surfaceShading(pixel, light, visibility);
    }
}
```

### 8.4 Sol como Area Light

```glsl
vec3 sampleSunAreaLight(const vec3 lightDirection) {
    if (sun.w >= 0.0) {
        float LoR = dot(lightDirection, shading_reflected);
        float d = sun.x;  // cos(angular radius)
        vec3 s = shading_reflected - LoR * lightDirection;
        return LoR < d ? normalize(lightDirection * d + normalize(s) * sun.y) : shading_reflected;
    }
    return lightDirection;
}
```

---

## PARTE 9: SISTEMA DE MATERIAIS (Arquitetura)

### 9.1 Como o Desenvolvedor Cria um Material

O Filament usa arquivos `.mat` (JSON-like) que são compilados pelo `matc`:

```
material {
    name : "Standard Lit Material",
    parameters : [
        { type : sampler2d, name : albedoMap },
        { type : sampler2d, name : normalMap },
        { type : sampler2d, name : roughnessMap },
        { type : sampler2d, name : metallicMap },
        { type : float, name : roughnessMultiplier, default : 1.0 }
    ],
    requires : [ uv0 ],
    shadingModel : lit,
    blending : opaque
}

fragment {
    void material(inout MaterialInputs material) {
        prepareMaterial(material);
        
        material.baseColor = texture(materialParams_albedoMap, getUV0());
        material.normal = texture(materialParams_normalMap, getUV0()).xyz * 2.0 - 1.0;
        material.roughness = texture(materialParams_roughnessMap, getUV0()).r 
                           * materialParams.roughnessMultiplier;
        material.metallic = texture(materialParams_metallicMap, getUV0()).r;
    }
}
```

### 9.2 O que o Pipeline Faz Automaticamente

O sistema injeta automaticamente:
1. **Estrutura MaterialInputs** - inicializada com valores default
2. **Função prepareMaterial()** - setup de normais e tangentes
3. **Todo o pipeline de iluminação** (BRDF, IBL, shadows, fog)
4. **Post-processing** (tone mapping, bloom, etc.)

### 9.3 Shading Models Disponíveis

| Model | Uso | Características |
|-------|-----|-----------------|
| `lit` | Padrão PBR | Difuso + Especular + Clear Coat + Anisotropia |
| `cloth` | Tecidos | Charlie NDF + Neubelt visibility |
| `subsurface` | Pele/Translúcidos | Subsurface scattering |
| `unlit` | UI/Emissivos | Sem iluminação |
| `specularGlossiness` | Legado | Workflow spec/gloss |

### 9.4 Blending Modes

| Mode | Uso |
|------|-----|
| `opaque` | Objetos sólidos |
| `transparent` | Vidro, água |
| `fade` | Fade in/out |
| `add` | Partículas aditivas |
| `masked` | Alpha testing |

### 9.5 MaterialBuilder API (C++)

```cpp
MaterialBuilder builder;
builder
    .name("MyMaterial")
    .shading(Shading::LIT)
    .blending(BlendingMode::OPAQUE)
    .parameter("baseColor", UniformType::FLOAT4)
    .parameter("roughness", UniformType::FLOAT)
    .parameter("metallic", UniformType::FLOAT)
    .parameter("albedoMap", SamplerType::SAMPLER_2D)
    .material(R"(
        void material(inout MaterialInputs material) {
            prepareMaterial(material);
            material.baseColor = materialParams.baseColor * texture(materialParams_albedoMap, getUV0());
            material.roughness = materialParams.roughness;
            material.metallic = materialParams.metallic;
        }
    )");
    
Package package = builder.build();
```

---

## PARTE 10: POST-PROCESSING

### 10.1 Pipeline de Post-Processing

```
HDR Color → Bloom → SSR → Color Grading → Tone Mapping → Dithering → Output
```

### 10.2 Bloom (Threshold + Blur)

O bloom é aplicado em múltiplas resoluções:
1. Threshold pass - extrai pixels brilhantes
2. Downsample chain (1/2, 1/4, 1/8...)
3. Blur em cada nível (separable Gaussian ou Kawase)
4. Upsample + blend

### 10.3 Tone Mapping

Operadores disponíveis:
- **ACES** (Academy Color Encoding System)
- **Filmic** (Uncharted 2 style)
- **Reinhard**
- **Display Range** (linear clamp)

### 10.4 Color Grading

Suporta:
- Exposure adjustment
- White balance
- Contrast/Saturation/Vibrance
- Shadows/Midtones/Highlights
- Channel mixer
- 3D LUT

### 10.5 Anti-Aliasing

| Técnica | Qualidade | Performance |
|---------|-----------|-------------|
| FXAA | Baixa | Muito Alta |
| TAA | Alta | Média |
| MSAA | Muito Alta | Baixa |

---

## PARTE 11: CHECKLIST DE IMPLEMENTAÇÃO

### 11.1 Fase 1: Fundamentos
- [ ] Estrutura MaterialInputs
- [ ] Estrutura PixelParams  
- [ ] Estrutura Light
- [ ] Funções BRDF (D_GGX, V_Smith, F_Schlick)
- [ ] Diffuse BRDF (Lambert)
- [ ] Cálculo de F0 e diffuseColor

### 11.2 Fase 2: Shading
- [ ] surfaceShading() básico
- [ ] Energy compensation
- [ ] Clear coat layer
- [ ] Anisotropia (opcional)

### 11.3 Fase 3: Image-Based Lighting
- [ ] Spherical Harmonics (9 coefs)
- [ ] Prefiltered radiance cubemap
- [ ] DFG LUT (pode ser pré-computado offline)
- [ ] evaluateIBL()

### 11.4 Fase 4: Shadows
- [ ] Shadow map rendering
- [ ] PCF sampling (3x3 mínimo)
- [ ] Cascaded shadow maps (direcional)
- [ ] Contact shadows (opcional)

### 11.5 Fase 5: AO
- [ ] SSAO pass
- [ ] Specular AO (Lagarde)
- [ ] Multi-bounce AO (opcional)

### 11.6 Fase 6: Luzes
- [ ] Luz direcional
- [ ] Luzes pontuais com atenuação
- [ ] Spotlights
- [ ] Froxel culling (opcional para muitas luzes)

### 11.7 Fase 7: Efeitos
- [ ] Fog atmosférico
- [ ] Bloom
- [ ] Tone mapping
- [ ] Dithering

---

## REFERÊNCIAS

### Papers Citados
1. Walter et al. 2007 - "Microfacet Models for Refraction through Rough Surfaces"
2. Heitz 2014 - "Understanding the Masking-Shadowing Function in Microfacet-Based BRDFs"
3. Schlick 1994 - "An Inexpensive BRDF Model for Physically-Based Rendering"
4. Burley 2012 - "Physically-Based Shading at Disney"
5. Kulla and Conty 2017 - "Revisiting Physically Based Shading at Imageworks"
6. Lagarde and de Rousiers 2014 - "Moving Frostbite to PBR"
7. Jimenez et al. 2016 - "Practical Realtime Strategies for Accurate Indirect Occlusion"
8. Estevez and Kulla 2017 - "Production Friendly Microfacet Sheen BRDF"
9. Neubelt and Pettineo 2013 - "Crafting a Next-gen Material Pipeline for The Order: 1886"
10. Donnelly and Lauritzen 2006 - "Variance Shadow Maps"

### Arquivos Fonte Principais (Filament)
- `shaders/src/surface_brdf.fs`
- `shaders/src/surface_shading_model_standard.fs`
- `shaders/src/surface_shading_lit.fs`
- `shaders/src/surface_light_indirect.fs`
- `shaders/src/surface_shadowing.fs`
- `shaders/src/surface_fog.fs`
- `shaders/src/surface_ambient_occlusion.fs`
- `libs/filamat/src/MaterialBuilder.cpp`

---

*Fim da Documentação*
