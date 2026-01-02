# Tonemapping Pass

The Tonemapping pass converts HDR (High Dynamic Range) colors to LDR (Low Dynamic Range) for display, while preserving visual detail across the brightness range.

---

## Overview

HDR rendering allows for realistic light intensities, but displays can only show LDR. Tonemapping compresses the HDR range while maintaining visual quality:
- Bright areas don't clip to white
- Dark areas remain visible
- Color relationships preserved

---

## Configuration

```cpp
auto* tonemap = pipeline.GetPass<se::TonemappingPass>();

tonemap->SetExposure(1.0f);
tonemap->SetGamma(2.2f);
tonemap->SetOperator(se::TonemapOperator::ACES);
```

### Properties

| Property | Type | Range | Default | Description |
|----------|------|-------|---------|-------------|
| `exposure` | `float` | 0.1-10.0 | 0.898 | Brightness multiplier |
| `gamma` | `float` | 1.0-3.0 | 1.161 | Gamma correction |
| `tonemapOp` | `TonemapOperator` | - | ACES | Tonemap curve |

---

## Tonemap Operators

```cpp
enum class TonemapOperator {
    ACES,        // Industry standard, cinematic
    Reinhard,    // Simple, preserves colors
    Uncharted2,  // Filmic, strong contrast
    Neutral,     // Minimal processing
    AgX          // Modern, color-accurate
};
```

### Comparison

| Operator | Look | Best For |
|----------|------|----------|
| **ACES** | Cinematic, rich | Most games |
| **Reinhard** | Natural, soft | Realistic scenes |
| **Uncharted2** | High contrast | Action games |
| **Neutral** | Minimal change | Debug, screenshots |
| **AgX** | Color-accurate | Photorealistic |

---

## Example

```cpp
// Cinematic look
tonemap->SetOperator(se::TonemapOperator::ACES);
tonemap->SetExposure(1.2f);

// Bright, saturated
tonemap->SetOperator(se::TonemapOperator::Reinhard);
tonemap->SetExposure(1.5f);

// High contrast action
tonemap->SetOperator(se::TonemapOperator::Uncharted2);
tonemap->SetExposure(0.8f);

// Raw output for debugging
tonemap->SetOperator(se::TonemapOperator::Neutral);
```

---

## Exposure

Exposure controls overall brightness:

```cpp
// Bright scene (outdoor, daytime)
tonemap->SetExposure(0.5f);

// Dark scene (indoor, night)
tonemap->SetExposure(2.0f);

// Auto-exposure (future feature)
// Currently manual only
```

---

## Gamma

Gamma correction compensates for display non-linearity:

```cpp
// Standard sRGB
tonemap->SetGamma(2.2f);

// Brighter midtones
tonemap->SetGamma(1.8f);
```

---

## ImGui Controls

```cpp
tonemap->RenderUI();
```

Shows:
- Exposure slider
- Gamma slider
- Operator dropdown

---

## Algorithm (ACES)

The ACES curve implemented:

```glsl
vec3 ACESTonemap(vec3 color) {
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    
    return clamp((color * (a * color + b)) / 
                 (color * (c * color + d) + e), 0.0, 1.0);
}
```

---

## Performance

- Very lightweight pass
- Single fullscreen quad
- No significant impact on frame rate

---

## See Also

- [Post-Processing Overview](Overview.md)
- [Color Grading](ColorGrading.md)
- [Bloom](Bloom.md)
