# Iridescent Microfacet BRDF Implementation

This document describes the implementation of an iridescent microfacet BRDF material based on thin-film interference, using the GGX microfacet distribution.

## Overview

The iridescent material simulates the colorful interference patterns seen on soap bubbles, oil slicks, and certain biological structures (like butterfly wings). This is achieved by modeling light interference in a thin transparent film layer on top of a base material.

## Theory

The material consists of three layers:
1. Air (refractive index = 1.0)
2. Thin film layer (refractive index = eta2, thickness = Dinc)
3. Base material (refractive index = eta3, extinction coefficient = kappa3)

Light reflecting from the top and bottom interfaces of the thin film interferes constructively or destructively depending on:
- The optical path difference (OPD = Dinc × cos(θ₂))
- The wavelength of light
- The viewing angle

## Parameters

The `IridescentMaterial` class takes the following parameters:

- **Dinc**: Thin film thickness in micrometers (typically 0.1 to 1.0)
  - Smaller values (0.1-0.3): Blue/violet iridescence
  - Medium values (0.3-0.5): Green/yellow iridescence
  - Larger values (0.5-1.0): Red/orange iridescence

- **eta2**: Refractive index of the thin film layer (typically 1.0-3.0)
  - Common values: 1.5 (similar to glass), 2.0 (stronger effect)
  - When Dinc → 0, eta2 is smoothly interpolated to 1.0 to avoid artifacts

- **eta3**: Refractive index of the base material (typically 1.0-5.0)
  - Determines the reflection characteristics of the underlying surface
  - Higher values create stronger reflections

- **kappa3**: Extinction coefficient of the base material (0.0 for dielectrics)
  - 0.0: Dielectric base (no absorption)
  - > 0.0: Conductor base (with absorption)

- **alpha_x, alpha_z**: Microfacet roughness parameters (0.01-1.0)
  - 0.01: Very smooth, mirror-like surface
  - 0.1-0.3: Glossy surface
  - > 0.5: Rough, matte surface

- **base_color** (optional): RGB base diffuse color for dielectric substrates (default: white)
  - Only affects dielectric substrates (kappa3 ≈ 0)
  - Modulates the iridescent reflection with a colored base
  - Useful for colored materials like beetle shells, feathers, or paint

## Usage Example

```cpp
#include <material/iridescentMaterial.hpp>

// Create an iridescent soap bubble effect
auto soapBubble = new pbrt::IridescentMaterial{
    0.3f,    // Dinc: 300nm film thickness
    1.33f,   // eta2: water refractive index
    1.0f,    // eta3: air on other side
    0.0f,    // kappa3: dielectric
    0.01f,   // alpha_x: very smooth
    0.01f    // alpha_z: very smooth
};

// Create an iridescent metal surface
auto iridescentMetal = new pbrt::IridescentMaterial{
    0.5f,    // Dinc: 500nm film thickness
    2.0f,    // eta2: oxide layer
    3.0f,    // eta3: metal base
    2.0f,    // kappa3: conductor
    0.1f,    // alpha_x: slightly rough
    0.1f     // alpha_z: slightly rough
};

// Create a colored dielectric substrate (e.g., green beetle shell)
auto greenBeetle = new pbrt::IridescentMaterial{
    0.4f,                      // Dinc: 400nm film thickness
    1.5f,                      // eta2: chitin layer
    1.5f,                      // eta3: dielectric base
    0.0f,                      // kappa3: dielectric
    0.05f,                     // alpha_x: glossy
    0.05f,                     // alpha_z: glossy
    glm::vec3(0.1f, 0.6f, 0.2f) // base_color: green substrate
};

// Add to scene
pbrt::Sphere sphere{{0.f, 0.f, 0.f}, 1.f};
scene.AddShape(sphere, soapBubble, {0.f, 1.f, 0.f}, {1.f, 1.f, 1.f});
```

## Implementation Details

### Fresnel Equations

The implementation includes separate Fresnel equations for:
- **Dielectric interfaces** (`FresnelDielectric`): Handles reflection/transmission at non-absorbing interfaces
- **Conductor interfaces** (`FresnelConductor`): Handles reflection at metallic surfaces with complex refractive indices

### Color Computation

The `EvalSensitivity` function computes XYZ tristimulus values using Gaussian approximations of the CIE color matching functions in Fourier space. This efficiently evaluates the spectral response to thin-film interference.

The XYZ values are converted to RGB using the CIE 1931 color space transformation matrix.

### Microfacet Distribution

The material uses the existing `Microfacet` class which implements:
- GGX (Trowbridge-Reitz) normal distribution
- Smith height-correlated masking-shadowing function
- Visible normal sampling for importance sampling

## References

This implementation is based on:
- "A Practical Extension to Microfacet Theory for the Modeling of Varying Iridescence" by Laurent Belcour and Pascal Barla (2017)
- Thin-film interference physics
- BRDF Explorer shader format

## Integration

The material is automatically compiled with the core library. To use it:

1. Include the header: `#include <material/iridescentMaterial.hpp>`
2. Create material instances with desired parameters
3. Assign to shapes in your scene
4. Render with MIS or PT renderer for best results

The `AdvanceMaterial` sample demonstrates various iridescent effects with different parameter values.
