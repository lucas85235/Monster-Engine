# Sistema PBR Robusto - Baseado no Filament

> ⚠️ **IMPORTANTE**: Sempre consultar `pbr_document.md` (raiz do projeto) para os cálculos exatos. NÃO inventar fórmulas próprias.

---

## Fase 1: Estruturas de Dados Core (Shaders)
📖 **Consultar**: `pbr_document.md` → PARTE 1.3 (Estruturas) + PARTE 2 (BRDF)

- [ ] 1.1 Criar `pbr/pbr_types.glsl` com estruturas MaterialInputs, PixelParams, Light
- [ ] 1.2 Atualizar `pbr/pbr_common.glsl` - BRDF Core (D_GGX, V_SmithGGX, F_Schlick, Fd_Lambert, Fd_Burley)
- [ ] 1.3 Cálculo de F0: `computeDielectricF0`, `computeF0`, `computeDiffuseColor`

---

## Fase 2: Shading Models
📖 **Consultar**: `pbr_document.md` → PARTE 3 (Shading Models)

- [ ] 2.1 Criar `pbr/shading_standard.glsl` com `surfaceShading()`, `isotropicLobe()`, `diffuseLobe()`
- [ ] 2.2 Clear Coat Layer: `clearCoatLobe()` com V_Kelemen, IOR 1.5
- [ ] 2.3 Sheen Layer (Cloth): `sheenLobe()` com D_Charlie + V_Neubelt
- [ ] 2.4 Energy Compensation (Kulla-Conty 2017): `energyCompensation = 1.0 + f0 * (1.0 / dfg.y - 1.0)`

---

## Fase 3: IBL (Image-Based Lighting)
📖 **Consultar**: `pbr_document.md` → PARTE 4 (IBL)

- [ ] 3.1 Spherical Harmonics 9 coeficientes: `Irradiance_SphericalHarmonics(n)`
- [ ] 3.2 DFG LUT ou `prefilteredDFG_Karis(NoV, roughness)` analítico
- [ ] 3.3 Specular IBL: `perceptualRoughnessToLod()`, `prefilteredRadiance()`
- [ ] 3.4 `evaluateIBL()` completo com AO integration

---

## Fase 4: Luzes Punctuais
📖 **Consultar**: `pbr_document.md` → PARTE 8 (Sistema de Luzes)

- [ ] 4.1 Atenuação física: `getSquareFalloffAttenuation()`, `getDistanceAttenuation()`
- [ ] 4.2 Spotlight: `getAngleAttenuation()`
- [ ] 4.3 Point Light com BRDF + atenuação
- [ ] 4.4 LightBuffer para múltiplas luzes

---

## Fase 5: Shadows
📖 **Consultar**: `pbr_document.md` → PARTE 5 (Shadow Mapping)

- [ ] 5.1 PCF 5x5 otimizado
- [ ] 5.2 Micro-Shadowing (Chan 2018): `computeMicroShadowing(NoL, visibility)`
- [ ] 5.3 Contact Shadows screen-space (opcional)

---

## Fase 6: Ambient Occlusion
📖 **Consultar**: `pbr_document.md` → PARTE 7 (AO)

- [ ] 6.1 Specular AO (Lagarde): `SpecularAO_Lagarde(NoV, visibility, roughness)`
- [ ] 6.2 Multi-Bounce AO (Jimenez 2016): `gtaoMultiBounce(visibility, albedo)`

---

## Fase 7: Fog Atmosférico
📖 **Consultar**: `pbr_document.md` → PARTE 6 (Fog)

- [ ] 7.1 Fog Exponencial com Altura (Beer-Lambert)
- [ ] 7.2 Sun Inscattering

---

## Fase 8: Sistema de Materiais C++
📖 **Consultar**: `pbr_document.md` → PARTE 9 (Sistema de Materiais)

- [ ] 8.1 Expandir `PBRMaterialParams` com clearCoat, anisotropy, sheen, subsurface
- [ ] 8.2 Criar `MaterialBuilder` API fluente
- [ ] 8.3 Material Presets (Metal, Plastic, Fabric, etc.)

---

## Fase 9: Post-Processing
📖 **Consultar**: `pbr_document.md` → PARTE 10 (Post-Processing)

- [ ] 9.1 Verificar Tone Mapping ACES
- [ ] 9.2 Dithering para banding

---

## Fase 10: Integração e Testes
📖 **Consultar**: `pbr_document.md` → Verificar todos os cálculos antes de integrar

- [ ] 10.1 Converter `SSGITestLayer` → `PBRTestLayer` (app de teste)
- [ ] 10.2 Atualizar `model.frag` para novo pipeline
- [ ] 10.3 Atualizar `instanced.frag`
- [ ] 10.4 Atualizar `skinned_model.frag`
- [ ] 10.5 Documentação final

---

## Referências

| Arquivo | Descrição |
|---------|-----------|
| **`pbr_document.md`** | Documentação completa com todos os cálculos (CONSULTAR SEMPRE!) |
| `assets/shaders/pbr/` | Shaders PBR atuais |
| `engine/include/engine/renderer/PBRMaterial.h` | Classe de material PBR |
| `apps/ssgi_test/` | App de teste (converter para PBR test) |
