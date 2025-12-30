# PBR System Implementation Checklist

## Status: ✅ Core Pipeline Complete

---

### Fase 1: Estruturas de Dados ✅
- [x] `pbr_types.glsl` - MaterialInputs, PixelParams, Light, ShadingData
- [x] `initMaterialInputs()`, `initShadingData()`, `createDirectionalLight()`

### Fase 2: BRDF Functions ✅
- [x] `D_GGX`, `D_GGX_Anisotropic`, `D_Charlie`
- [x] `V_SmithGGXCorrelated`, `V_Kelemen`, `V_Neubelt`
- [x] `F_Schlick`, `Fd_Lambert`, `Fd_Burley`
- [x] Energy Compensation (Kulla-Conty 2017)

### Fase 3: Shading Pipeline ✅
- [x] `getPixelParams()` - MaterialInputs → PixelParams
- [x] `surfaceShading()` - Full Filament pipeline
- [x] `specularLobe()`, `diffuseLobe()`
- [x] `clearCoatLobe()`, `sheenLobe()`

### Fase 4-7: Lighting & Effects ✅
- [x] Point/Spot light evaluation
- [x] PCF 5x5 shadows
- [x] `computeMicroShadowing()` (Chan 2018)
- [x] `multiBounceAO()`, `specularAO()`
- [x] Height fog with sun inscattering

### Fase 8: C++ Material System ✅
- [x] `PBRMaterialParams` expanded with advanced params
- [x] `BindPBR()` updated
- [x] **Material Override System** in SceneRenderer

### Fase 9: Post-Processing ✅
- [x] ACES Tone Mapping
- [x] Gamma correction

### Fase 10: Shader Integration ✅
- [x] **model.frag** - Full Filament pipeline
- [x] **instanced.frag** - Full Filament pipeline
- [x] **skinned_model.frag** - Full Filament pipeline
- [x] **PBRTestLayer** with toggle and presets

---

### Remaining/Optional:
- [ ] Shadow Cascades (CSM)
- [ ] HDR IBL from cubemaps (HDR files available in assets/textures/ibl)
- [ ] DFG LUT texture
- [ ] Contact Shadows
- [ ] Dithering
