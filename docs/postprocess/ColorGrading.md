# Color Grading Pass

The Color Grading pass adjusts colors for artistic effect, including exposure, contrast, saturation, white balance, and vignette.

---

## Overview

Color grading is applied after bloom but before tonemapping, operating in HDR space for maximum quality.

---

## Configuration

```cpp
auto* cg = pipeline.GetPass<se::ColorGradingPass>();

// Basic adjustments
cg->Exposure() = 1.0f;
cg->Contrast() = 1.1f;
cg->Saturation() = 1.05f;
cg->Brightness() = 0.0f;

// White balance
cg->Temperature() = 0.0f;
cg->Tint() = 0.0f;

// Tone adjustments
cg->Shadows() = 0.0f;
cg->Highlights() = 0.0f;

// Vignette
cg->VignetteIntensity() = 0.3f;
cg->VignetteFalloff() = 0.5f;
```

---

## Properties

| Property | Type | Range | Default | Description |
|----------|------|-------|---------|-------------|
| `Exposure` | `float` | 0.0-5.0 | 0.898 | Brightness multiplier |
| `Contrast` | `float` | 0.0-3.0 | 1.103 | Contrast adjustment |
| `Saturation` | `float` | 0.0-3.0 | 0.988 | Color saturation |
| `Brightness` | `float` | -1.0-1.0 | 0.021 | Additive brightness |
| `Temperature` | `float` | -1.0-1.0 | 0.0 | Warm (positive) / Cool (negative) |
| `Tint` | `float` | -1.0-1.0 | 0.0 | Green (negative) / Magenta (positive) |
| `Shadows` | `float` | -1.0-1.0 | -0.001 | Adjust dark areas |
| `Highlights` | `float` | -1.0-1.0 | 0.011 | Adjust bright areas |
| `VignetteIntensity` | `float` | 0.0-1.0 | 0.726 | Vignette darkness |
| `VignetteFalloff` | `float` | 0.0-1.0 | 0.347 | Vignette size |

---

## Common Presets

### Cinematic Warm

```cpp
cg->Temperature() = 0.15f;
cg->Contrast() = 1.15f;
cg->Saturation() = 0.9f;
cg->VignetteIntensity() = 0.3f;
cg->Highlights() = -0.1f;
```

### Horror/Cold

```cpp
cg->Temperature() = -0.2f;
cg->Saturation() = 0.7f;
cg->Contrast() = 1.2f;
cg->Shadows() = -0.1f;
cg->VignetteIntensity() = 0.5f;
```

### Vintage

```cpp
cg->Temperature() = 0.1f;
cg->Tint() = 0.05f;
cg->Saturation() = 0.8f;
cg->Contrast() = 0.9f;
cg->VignetteIntensity() = 0.4f;
cg->VignetteFalloff() = 0.6f;
```

### High Contrast Action

```cpp
cg->Contrast() = 1.3f;
cg->Saturation() = 1.1f;
cg->Shadows() = -0.15f;
cg->Highlights() = 0.1f;
```

---

## Vignette

Darkens the edges of the screen:

```cpp
// Subtle vignette
cg->VignetteIntensity() = 0.2f;
cg->VignetteFalloff() = 0.5f;

// Strong vignette
cg->VignetteIntensity() = 0.6f;
cg->VignetteFalloff() = 0.3f;

// No vignette
cg->VignetteIntensity() = 0.0f;
```

---

## White Balance

Temperature shifts colors warm or cool:

```cpp
cg->Temperature() = 0.2f;   // Warmer (sunset)
cg->Temperature() = -0.2f;  // Cooler (moonlight)
```

Tint adjusts green/magenta:

```cpp
cg->Tint() = 0.1f;   // More magenta
cg->Tint() = -0.1f;  // More green
```

---

## ImGui Controls

```cpp
cg->RenderUI();
```

Provides sliders for all adjustments.

---

## Order in Pipeline

Color grading should come:
- **After** Bloom (so bloom is affected by grading)
- **Before** Tonemapping (to work in HDR)

```cpp
pipeline.AddPass<se::BloomPass>();
pipeline.AddPass<se::FXAAPass>();
pipeline.AddPass<se::ColorGradingPass>();  // Here
pipeline.AddPass<se::TonemappingPass>();
```

---

## See Also

- [Post-Processing Overview](Overview.md)
- [Tonemapping](Tonemapping.md)
