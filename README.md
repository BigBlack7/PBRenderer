# PBRenderer

- Refer to PBRT-v4
- Build it by msvc

# NameFormat

- file name(xxXxx)
- class name(XxXxx)
- class member var name(mXxx)
- struct name(XxXxx)
- struct member var name(--xxXxx--)
- all funcs name(XxXxx)
- temporary var or func var(xx_xxx)

# Implemented

- BVH
- AreaLight & EnvLight
- Conductor & Dielectric
- Microfacet Theory
- PTRenderer
- MIS
- SFML Window
- Easy Threadpool
- Logger
- Tone Mapping（Gamma / Reinhard / ACES）

# Todo

- BDPT
- VPT
- SSS
- Medium
- Sobol Sampler
- Spectrum
- Scene Parser
- Imgui Controller
- Transform

# Tone Mapping 教学（中文）

目前渲染器支持 3 种 tone mapping（在 `core/utils/rgb.hpp`）：

1. `Gamma`（默认）
   - 先把线性颜色 clamp 到 `[0,1]`，再做 `gamma=2.2` 编码。
   - 适合快速预览，行为与历史版本保持一致。
2. `Reinhard`
   - 公式：`c = c / (1 + c)`，可压高光，减少过曝。
3. `ACES`
   - 使用常见 ACES 近似曲线：
     `c = (c*(2.51*c+0.03)) / (c*(2.43*c+0.59)+0.14)`
   - 高光 roll-off 更自然，观感更“电影感”。

## 怎么用

在渲染前设置一次全局 tone mapping 模式即可：

```cpp
#include <utils/rgb.hpp>

// 默认行为（兼容旧代码）
pbrt::RGB::SetToneMapping(pbrt::RGB::ToneMappingType::Gamma, 1.0f);

// Reinhard（曝光 1.0）
pbrt::RGB::SetToneMapping(pbrt::RGB::ToneMappingType::Reinhard, 1.0f);

// ACES（可适当提高曝光）
pbrt::RGB::SetToneMapping(pbrt::RGB::ToneMappingType::ACES, 1.2f);
```

说明：
- `exposure` 会在 tone mapping 前乘到线性颜色上。
- 内部会对负值做保护，并在输出前 clamp 到 `[0,1]`，避免无效颜色。

# Gallery(1024 spp)

![CornellboxSeries](gallery/cornellbox01.png)
![CornellboxSeries](gallery/cornellbox02.png)
![CornellboxSeries](gallery/cornellbox03.png)
![CornellboxSeries](gallery/cornellbox04.png)
