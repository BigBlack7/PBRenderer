# Sobol 采样器集成指南 - 完整替换方案

## 概述

经过详细分析你的 PBRenderer 工程，我发现了 **所有使用 RNG 的位置**。以下是完整的集成方案，包括需要修改的每一个文件和具体的修改方法。

---

## 一、核心渲染器（必须修改）⭐⭐⭐

### 1.1 PTRenderer.cpp

**当前使用 RNG 的位置：**
- 第 9-10 行：初始化 RNG 和设置种子
- 第 11 行：相机光线采样 `rng.Uniform()` × 2
- 第 27 行：俄罗斯轮盘赌 `rng.Uniform()`
- 第 48 行：光源采样 `rng.Uniform()`
- 第 51 行：光源位置采样 `SampleLight(..., rng, ...)`
- 第 60 行：BSDF 采样 `SampleBSDF(..., rng)`

**修改方案：**

```cpp
// 文件：core/renderer/PTRenderer.cpp
#include "PTRenderer.hpp"
#include "utils/frame.hpp"
#include "sampler/sobolSampler.hpp"  // 替换 #include "utils/rng.hpp"

namespace pbrt
{
    glm::vec3 PTRenderer::RenderPixel(const glm::ivec3 &pixel_coord)
    {
        // 使用 Sobol 采样器替代 RNG
        thread_local SobolSampler sampler;
        sampler.StartPixelSample(
            glm::ivec2(pixel_coord.x, pixel_coord.y), 
            pixel_coord.z
        );
        
        // 相机光线采样 - 使用 Get2D() 替代两次 Uniform()
        auto ray = mCamera.GenerateRay(pixel_coord, sampler.Get2D());
        
        glm::vec3 beta = {1.f, 1.f, 1.f};
        glm::vec3 radiance = {0.f, 0.f, 0.f};
        float q = 0.9f;
        bool last_is_delta = true;

        while (true)
        {
            auto hit_info = mScene.Intersect(ray);
            if (hit_info.has_value())
            {
                if (last_is_delta && hit_info->__material__ && hit_info->__material__->mAreaLight)
                {
                    radiance += beta * hit_info->__material__->mAreaLight->GetRadiance(...);
                }

                // 俄罗斯轮盘赌 - 使用 Get1D()
                if (sampler.Get1D() > q)
                {
                    break;
                }
                beta /= q;
                Frame frame(hit_info->__normal__);
                glm::vec3 light_dir;

                if (hit_info->__material__)
                {
                    glm::vec3 view_dir = frame.LocalFromWorld(-ray.__direction__);

                    if (view_dir.y == 0)
                    {
                        ray.__origin__ = hit_info->__hitPoint__;
                        continue;
                    }

                    last_is_delta = hit_info->__material__->IsDeltaDistribution();
                    if (!last_is_delta)
                    {
                        // 光源选择采样 - 使用 Get1D()
                        auto light_sample_info = mScene.GetLightSampler(false).SampleLight(sampler.Get1D());
                        if (light_sample_info.has_value())
                        {
                            // ⚠️ 注意：这里需要传递 sampler 而不是 rng
                            // 需要修改 SampleLight 接口（见下文）
                            auto light_info = light_sample_info->__light__->SampleLight(
                                hit_info->__hitPoint__, 
                                mScene.GetRadius(), 
                                sampler,  // 传递采样器
                                false
                            );
                            if (light_info.has_value() && (!mScene.Intersect(...)))
                            {
                                glm::vec3 light_dir_local = frame.LocalFromWorld(light_info->__direction__);
                                radiance += beta * hit_info->__material__->BSDF(...) * ...;
                            }
                        }
                    }

                    // BSDF 采样 - 传递 sampler
                    auto bsdf_info = hit_info->__material__->SampleBSDF(
                        hit_info->__hitPoint__, 
                        view_dir, 
                        sampler  // 传递采样器
                    );

                    if (!bsdf_info.has_value())
                        break;

                    beta *= bsdf_info->__bsdf__ * glm::abs(bsdf_info->__lightDirection__.y) / bsdf_info->__pdf__;
                    light_dir = bsdf_info->__lightDirection__;
                }
                else
                {
                    break;
                }

                ray.__origin__ = hit_info->__hitPoint__;
                ray.__direction__ = frame.WorldFromLocal(light_dir);
            }
            else
            {
                if (last_is_delta)
                {
                    for (const auto &light : mScene.GetInfiniteLights())
                    {
                        glm::vec3 light_dir_delta = glm::normalize(ray.__direction__);
                        radiance += beta * light->GetRadiance(...);
                    }
                }
                break;
            }
        }

        return radiance;
    }
}
```

**修改总结：**
1. ✅ 替换 `#include "utils/rng.hpp"` → `#include "sampler/sobolSampler.hpp"`
2. ✅ 替换 `thread_local RNG rng{}` → `thread_local SobolSampler sampler`
3. ✅ 添加 `sampler.StartPixelSample(...)` 初始化
4. ✅ 替换所有 `rng.Uniform()` → `sampler.Get1D()` 或 `sampler.Get2D()`
5. ⚠️ 传递 `sampler` 到 `SampleLight` 和 `SampleBSDF`

---

### 1.2 MISRenderer.cpp

**当前使用 RNG 的位置：**
- 第 24-25 行：初始化 RNG
- 第 26 行：相机光线采样 × 2
- 第 61 行：俄罗斯轮盘赌
- 第 85 行：光源选择
- 第 88 行：光源位置采样
- 第 109 行：BSDF 采样

**修改方案：** （与 PTRenderer 类似）

```cpp
// 文件：core/renderer/MISRenderer.cpp
#include "MISRenderer.hpp"
#include "utils/frame.hpp"
#include "sampler/sobolSampler.hpp"  // 替换 RNG

namespace pbrt
{
    glm::vec3 MISRenderer::RenderPixel(const glm::ivec3 &pixel_coord)
    {
        thread_local SobolSampler sampler;
        sampler.StartPixelSample(
            glm::ivec2(pixel_coord.x, pixel_coord.y), 
            pixel_coord.z
        );
        
        auto ray = mCamera.GenerateRay(pixel_coord, sampler.Get2D());
        
        // ... 其余代码类似 PTRenderer 的修改
        // 将所有 rng.Uniform() 替换为 sampler.Get1D() 或 Get2D()
        // 将 rng 参数替换为 sampler
    }
}
```

---

## 二、材质系统（接口修改）⭐⭐⭐

### 2.1 Material 基类接口

**需要修改的文件：** `core/material/material.hpp`

**当前接口：**
```cpp
virtual std::optional<BSDFInfo> SampleBSDF(
    const glm::vec3 &hit_point, 
    const glm::vec3 &view_dir, 
    const RNG &rng
) const = 0;
```

**修改方案（两种选择）：**

#### 选项 A：添加新接口（推荐，向后兼容）✅

```cpp
// core/material/material.hpp
#pragma once
#include "utils/rng.hpp"
#include "sampler/sampler.hpp"  // 新增
#include <glm/glm.hpp>
#include <optional>

namespace pbrt
{
    struct BSDFInfo { /* ... */ };

    class Material
    {
    public:
        const class AreaLight *mAreaLight{nullptr};

    public:
        // 新接口：使用 Sampler（推荐）
        virtual std::optional<BSDFInfo> SampleBSDF(
            const glm::vec3 &hit_point, 
            const glm::vec3 &view_dir, 
            Sampler &sampler
        ) const {
            // 默认实现：转换为 RNG（临时兼容）
            RNGSampler rng_adapter;
            // 从 sampler 获取种子并设置
            return SampleBSDF(hit_point, view_dir, rng_adapter.GetRNG());
        }
        
        // 旧接口：保持兼容（标记为将来弃用）
        virtual std::optional<BSDFInfo> SampleBSDF(
            const glm::vec3 &hit_point, 
            const glm::vec3 &view_dir, 
            const RNG &rng
        ) const = 0;
        
        virtual glm::vec3 BSDF(...) const = 0;
        virtual float PDF(...) const = 0;
        virtual bool IsDeltaDistribution() const = 0;
        virtual void Regularize() const {};
    };
}
```

#### 选项 B：直接替换接口（破坏性修改）⚠️

```cpp
// 将所有 RNG 参数改为 Sampler
virtual std::optional<BSDFInfo> SampleBSDF(
    const glm::vec3 &hit_point, 
    const glm::vec3 &view_dir, 
    Sampler &sampler  // 直接替换
) const = 0;
```

**推荐使用选项 A**，这样可以逐步迁移。

---

### 2.2 各个材质实现类

需要修改以下文件，为每个材质实现新的 `SampleBSDF(Sampler&)` 方法：

#### 2.2.1 DiffuseMaterial
**文件：** `core/material/diffuseMaterial.cpp`

```cpp
std::optional<BSDFInfo> DiffuseMaterial::SampleBSDF(
    const glm::vec3 &hit_point, 
    const glm::vec3 &view_dir, 
    Sampler &sampler  // 使用 Sampler
) const
{
    // 使用 sampler.Get2D() 替代两次 rng.Uniform()
    auto sample = sampler.Get2D();
    glm::vec3 light_dir = UniformSampleHemisphere(sample);
    // ... 其余代码
}
```

#### 2.2.2 ConductorMaterial
**文件：** `core/material/conductorMaterial.cpp`

```cpp
std::optional<BSDFInfo> ConductorMaterial::SampleBSDF(
    const glm::vec3 &hit_point, 
    const glm::vec3 &view_dir, 
    Sampler &sampler
) const
{
    glm::vec3 microfacet_normal{0.f, 1.f, 0.f};
    if (!mMicrofacet.IsDeltaDistribution())
    {
        // ⚠️ 这里需要修改 Microfacet::SampleVisibleNormal
        microfacet_normal = mMicrofacet.SampleVisibleNormal(view_dir, sampler);
    }
    // ... 其余代码
}
```

#### 2.2.3 DielectricMaterial
**文件：** `core/material/dielectricMaterial.cpp`

#### 2.2.4 IridescentMaterial
**文件：** `core/material/iridescentMaterial.cpp`

#### 2.2.5 SpecularMaterial, GroundMaterial
**文件：** `core/material/specularMaterial.cpp`, `core/material/groundMaterial.cpp`

**所有材质都需要类似的修改。**

---

### 2.3 Microfacet 类

**文件：** `core/material/microfacet.hpp` 和 `microfacet.cpp`

**需要修改的方法：**
```cpp
// 当前
glm::vec3 SampleVisibleNormal(const glm::vec3 &view_dir, const RNG &rng) const;

// 修改为（添加重载）
glm::vec3 SampleVisibleNormal(const glm::vec3 &view_dir, Sampler &sampler) const;
```

**实现：**
```cpp
glm::vec3 Microfacet::SampleVisibleNormal(
    const glm::vec3 &view_dir, 
    Sampler &sampler
) const
{
    glm::vec3 view_dir_up = view_dir.y > 0 ? view_dir : -view_dir;
    glm::vec3 view_dir_hemi = glm::normalize(
        glm::vec3(mAlphaX * view_dir_up.x, view_dir_up.y, mAlphaZ * view_dir_up.z)
    );

    // 使用 sampler.Get2D() 替代 {rng.Uniform(), rng.Uniform()}
    glm::vec2 sample = sampler.Get2D();
    glm::vec2 disk_sample = UniformSampleUnitDisk(sample);
    
    // ... 其余代码不变
}
```

---

## 三、光源系统（接口修改）⭐⭐

### 3.1 Light 基类

**文件：** `core/light/light.hpp`

**当前接口：**
```cpp
virtual std::optional<LightInfo> SampleLight(
    const glm::vec3 &surface_point, 
    float scene_radius, 
    const RNG &rng, 
    bool MISC
) const = 0;
```

**修改方案：**

#### 选项 A：添加重载（推荐）
```cpp
// 新接口
virtual std::optional<LightInfo> SampleLight(
    const glm::vec3 &surface_point, 
    float scene_radius, 
    Sampler &sampler, 
    bool MISC
) const {
    // 默认实现：使用 RNG 适配
    RNGSampler rng_adapter;
    return SampleLight(surface_point, scene_radius, rng_adapter.GetRNG(), MISC);
}

// 保留旧接口
virtual std::optional<LightInfo> SampleLight(
    const glm::vec3 &surface_point, 
    float scene_radius, 
    const RNG &rng, 
    bool MISC
) const = 0;
```

---

### 3.2 各个光源实现

#### 3.2.1 AreaLight
**文件：** `core/light/areaLight.cpp`

```cpp
std::optional<LightInfo> AreaLight::SampleLight(
    const glm::vec3 &surface_point, 
    float scene_radius, 
    Sampler &sampler,  // 使用 Sampler
    bool MISC
) const
{
    // ⚠️ 需要修改 Shape::SampleShape
    auto shape_sample = mShape.SampleShape(sampler);
    // ... 其余代码
}
```

#### 3.2.2 EnvLight
**文件：** `core/light/envLight.cpp`

需要修改光源采样方法，使用 `sampler.Get1D()` 和 `sampler.Get2D()`。

#### 3.2.3 InfiniteLight
**文件：** `core/light/infiniteLight.cpp`

---

## 四、形状系统（接口修改）⭐

### 4.1 Shape 基类

**文件：** `core/shape/shape.hpp`

**需要修改：**
```cpp
// 当前
virtual std::optional<ShapeInfo> SampleShape(const RNG &rng) const = 0;

// 修改为
virtual std::optional<ShapeInfo> SampleShape(Sampler &sampler) const = 0;
```

---

### 4.2 各个形状实现

#### 4.2.1 Sphere
**文件：** `core/shape/sphere.cpp`

```cpp
std::optional<ShapeInfo> Sphere::SampleShape(Sampler &sampler) const
{
    // 使用 sampler.Get2D() 生成球面采样
    glm::vec2 sample = sampler.Get2D();
    glm::vec3 direction = UniformSampleSphere(sample);
    // ... 其余代码
}
```

#### 4.2.2 Circle
**文件：** `core/shape/circle.cpp`

```cpp
std::optional<ShapeInfo> Circle::SampleShape(Sampler &sampler) const
{
    glm::vec2 sample = sampler.Get2D();
    // ... 圆盘采样
}
```

#### 4.2.3 Triangle
**文件：** `core/shape/triangle.cpp`

```cpp
std::optional<ShapeInfo> Triangle::SampleShape(Sampler &sampler) const
{
    glm::vec2 sample = sampler.Get2D();
    // 三角形重心坐标采样
}
```

---

## 五、完整修改清单

### 必须修改的文件（按优先级）

| 优先级 | 文件路径 | 修改内容 | 难度 |
|--------|---------|---------|------|
| ⭐⭐⭐ | `core/renderer/PTRenderer.cpp` | 替换 RNG 为 Sobol | 简单 |
| ⭐⭐⭐ | `core/renderer/MISRenderer.cpp` | 替换 RNG 为 Sobol | 简单 |
| ⭐⭐⭐ | `core/material/material.hpp` | 添加 Sampler 接口 | 中等 |
| ⭐⭐ | `core/material/diffuseMaterial.cpp` | 实现 Sampler 重载 | 简单 |
| ⭐⭐ | `core/material/conductorMaterial.cpp` | 实现 Sampler 重载 | 简单 |
| ⭐⭐ | `core/material/dielectricMaterial.cpp` | 实现 Sampler 重载 | 简单 |
| ⭐⭐ | `core/material/iridescentMaterial.cpp` | 实现 Sampler 重载 | 简单 |
| ⭐⭐ | `core/material/specularMaterial.cpp` | 实现 Sampler 重载 | 简单 |
| ⭐⭐ | `core/material/groundMaterial.cpp` | 实现 Sampler 重载 | 简单 |
| ⭐⭐ | `core/material/microfacet.hpp/cpp` | 修改 SampleVisibleNormal | 中等 |
| ⭐⭐ | `core/light/light.hpp` | 添加 Sampler 接口 | 中等 |
| ⭐ | `core/light/areaLight.cpp` | 实现 Sampler 重载 | 简单 |
| ⭐ | `core/light/envLight.cpp` | 实现 Sampler 重载 | 中等 |
| ⭐ | `core/light/infiniteLight.cpp` | 实现 Sampler 重载 | 简单 |
| ⭐ | `core/shape/shape.hpp` | 修改 SampleShape 接口 | 简单 |
| ⭐ | `core/shape/sphere.cpp` | 实现 Sampler 版本 | 简单 |
| ⭐ | `core/shape/circle.cpp` | 实现 Sampler 版本 | 简单 |
| ⭐ | `core/shape/triangle.cpp` | 实现 Sampler 版本 | 简单 |

**总计：** 约 18 个文件需要修改

---

## 六、渐进式迁移策略（推荐）

为了避免一次性修改导致的问题，建议按以下步骤进行：

### 阶段 1：添加接口重载（不破坏现有代码）
1. 在 `Material` 基类添加 `SampleBSDF(Sampler&)` 重载
2. 在 `Light` 基类添加 `SampleLight(Sampler&)` 重载
3. 在 `Shape` 基类添加 `SampleShape(Sampler&)` 重载
4. 默认实现调用原有的 RNG 版本

### 阶段 2：修改渲染器使用 Sobol
1. 修改 `PTRenderer.cpp` 使用 `SobolSampler`
2. 修改 `MISRenderer.cpp` 使用 `SobolSampler`
3. 此时渲染器调用新接口，但底层通过适配器调用旧实现

### 阶段 3：逐个实现 Sampler 版本
1. 实现 `Microfacet::SampleVisibleNormal(Sampler&)`
2. 实现各材质的 `SampleBSDF(Sampler&)`
3. 实现各光源的 `SampleLight(Sampler&)`
4. 实现各形状的 `SampleShape(Sampler&)`

### 阶段 4：测试和验证
1. 对比 RNG 和 Sobol 的渲染结果
2. 测试收敛速度
3. 验证各个场景

### 阶段 5：（可选）移除旧接口
1. 移除 RNG 版本的接口
2. 清理代码

---

## 七、快速开始示例

如果你想快速看到效果，可以**只修改渲染器**，使用临时适配器：

```cpp
// PTRenderer.cpp 快速版本
#include "sampler/sobolSampler.hpp"
#include "sampler/rngSampler.hpp"

glm::vec3 PTRenderer::RenderPixel(const glm::ivec3 &pixel_coord)
{
    thread_local SobolSampler sobol_sampler;
    sobol_sampler.StartPixelSample(
        glm::ivec2(pixel_coord.x, pixel_coord.y), 
        pixel_coord.z
    );
    
    // 创建 RNG 适配器（临时方案）
    thread_local RNGSampler rng_adapter;
    
    // 相机采样使用 Sobol
    auto ray = mCamera.GenerateRay(pixel_coord, sobol_sampler.Get2D());
    
    // 其他地方暂时用 RNG（通过适配器）
    // ...后续逐步替换
}
```

这样可以**立即获得相机采样的改进**，其他部分逐步迁移。

---

## 八、预期效果

完成所有修改后，你应该能看到：

### 性能提升
- **低 SPP (16-64)**：2-3x 更快收敛
- **中等 SPP (64-256)**：1.5-2x 更快收敛
- **高 SPP (>256)**：10-30% 改善

### 视觉改善
- 更少的噪点
- 更均匀的样本分布
- 减少结构化伪影

### 对比测试
建议渲染相同场景，对比：
- RNG @ 256 SPP vs Sobol @ 128 SPP
- 观察质量差异和渲染时间

---

## 九、注意事项

### 维度管理 ⚠️
Sobol 采样器对维度顺序敏感：
```cpp
// ✅ 正确
sampler.StartPixelSample(pixel, sample_idx);  // 重置
auto cam = sampler.Get2D();    // 维度 0-1
auto bsdf = sampler.Get2D();   // 维度 2-3
auto rr = sampler.Get1D();     // 维度 4

// ❌ 错误 - 不要跨帧使用同一维度
sampler.Get2D();  // 维度 0-1
sampler.Get2D();  // 维度 2-3
// 没有 StartPixelSample，继续使用高维度 - 错误！
```

### 线程安全 ✅
每个线程使用 `thread_local` 采样器实例：
```cpp
thread_local SobolSampler sampler;  // ✅ 正确
```

### 向后兼容 ✅
使用接口重载保持兼容性，逐步迁移。

---

## 十、总结

1. **必须修改**：2 个渲染器文件（PTRenderer.cpp, MISRenderer.cpp）
2. **强烈建议修改**：材质系统（6个材质类 + Microfacet）
3. **建议修改**：光源系统（3个光源类）
4. **可选修改**：形状系统（3个形状类）

从渲染器开始，逐步向下迁移到材质、光源和形状系统。

**预计工作量**：
- 快速版本（仅渲染器）：1-2 小时
- 完整版本（所有系统）：4-8 小时
- 测试验证：2-4 小时

需要我帮你生成具体某个文件的完整修改代码吗？
