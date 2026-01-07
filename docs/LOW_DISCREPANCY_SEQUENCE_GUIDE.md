# 低差异序列（Low-Discrepancy Sequence）使用指南

## 目录
1. [基本概念](#基本概念)
2. [当前实现分析](#当前实现分析)
3. [哪些地方应该使用低差异序列](#哪些地方应该使用低差异序列)
4. [哪些地方不应该使用低差异序列](#哪些地方不应该使用低差异序列)
5. [当前代码中的问题](#当前代码中的问题)
6. [修复建议与实现指导](#修复建议与实现指导)
7. [接口设计建议](#接口设计建议)

---

## 基本概念

### 什么是低差异序列？

低差异序列（如Sobol序列、Halton序列）是一类准随机数序列，与伪随机数相比：
- **优点**：在多维空间中分布更均匀，收敛速度更快（O(1/N) vs O(1/√N)）
- **缺点**：存在维度相关性，不适用于动态维度消耗的场景

### PBRT-v4中的采样器设计理念

1. **固定维度消耗模式**：每个样本应该消耗固定数量的维度
2. **像素相关性**：使用`SobolIntervalToIndex`将样本索引映射到像素位置
3. **Owen扰动**：使用Owen scrambling消除低维度的相关性

---

## 当前实现分析

### SobolSampler实现（sequence/sobolSampler.cpp）

当前实现基本正确，包含以下关键组件：

```cpp
// 正确的实现
1. SobolIntervalToIndex - 将像素和样本索引映射到Sobol序列索引
2. FastOwenScrambler - Owen扰动实现
3. SampleDimension - 根据维度采样
```

**问题**：维度计数器`mDimension`是`mutable`的，这意味着每次调用`Get1D()`或`Get2D()`都会增加维度。在路径追踪中，如果路径长度不固定，不同像素在同一样本索引下消耗的维度数不同，会导致采样质量下降。

### RNGSampler实现（sequence/rngSampler.hpp）

这是一个基于伪随机数的采样器，作为对照组，实现简单正确。

---

## 哪些地方应该使用低差异序列

### ✅ 1. 像素内采样偏移（Camera Ray Generation）

**位置**：`PTRenderer.cpp`和`MISRenderer.cpp`中的`GenerateRay`调用

**原因**：这是最关键的采样点，像素内的样本分布直接影响抗锯齿和收敛速度。

```cpp
// 推荐使用
sobol.StartPixelSample(glm::ivec2(pixel_coord.x, pixel_coord.y), pixel_coord.z);
auto ray = mCamera.GenerateRay(pixel_coord, sobol.Get2D()); // 维度0-1
```

### ✅ 2. BSDF采样方向（Material Sampling）

**位置**：`Material::SampleBSDF()`调用

**原因**：BSDF采样对于间接光照的收敛至关重要。

**注意**：这里需要谨慎，因为路径深度是动态的。

### ✅ 3. 光源采样（Light Sampling）

**位置**：
- `LightSampler::SampleLight(float u)` - 选择哪个光源
- `Light::SampleLight()` - 在光源表面采样点

**原因**：直接光照采样是NEE（Next Event Estimation）的关键。

---

## 哪些地方不应该使用低差异序列

### ❌ 1. 俄罗斯轮盘赌（Russian Roulette）

**位置**：`PTRenderer.cpp`和`MISRenderer.cpp`中的`rng.Uniform() > q`

**原因**：
- 俄罗斯轮盘赌是一个**二值决策**，不是连续采样
- 使用低差异序列会导致相邻像素的路径终止行为相关
- 这会引入**结构化噪声**，比随机噪声更难以消除

```cpp
// 错误：不要使用低差异序列
if (sampler.Get1D() > q) break;

// 正确：使用独立随机数
if (rng.Uniform() > q) break;
```

### ❌ 2. 反射/透射选择（Reflection/Transmission Decision）

**位置**：`DielectricMaterial::SampleBSDF()`中的`rng.Uniform() <= F`

**原因**：同上，这是二值决策，应使用随机数。

```cpp
// 当前实现是正确的
if (rng.Uniform() <= F) // 反射
```

### ❌ 3. 拒绝采样循环（Rejection Sampling）

**位置**：`spherical.hpp`中的`UniformSampleSphere`和`UniformSampleHemisphere`

**原因**：
- 拒绝采样会消耗**不确定数量**的随机数
- 低差异序列的维度关联性会在拒绝采样中被破坏
- 会导致采样质量下降

```cpp
// 错误：注释掉的实现是有问题的
inline glm::vec3 UniformSampleSphere(const Sampler &sequence)
{
    glm::vec3 res;
    do
    {
        res = {sequence.Get1D(), sequence.Get1D(), sequence.Get1D()}; // 每次迭代消耗3维度
        res = res * 2.f - 1.f;
    } while (glm::length(res) > 1.f); // 迭代次数不确定
    return glm::normalize(res);
}

// 正确：使用确定性维度消耗的参数化方法
inline glm::vec3 UniformSampleSphere(const glm::vec2 &u)
{
    float z = 1.f - 2.f * u.x;
    float r = glm::sqrt(glm::max(0.f, 1.f - z * z));
    float phi = 2.f * PI * u.y;
    return glm::vec3(r * glm::cos(phi), z, r * glm::sin(phi));
}
```

---

## 当前代码中的问题

### 问题1：MISRenderer中的Sobol使用方式

```cpp
// MISRenderer.cpp 第21-31行（已注释）
/*  TODO: 低差异序列会更加地不收敛, 待解决
    static std::once_flag sobol_extent_flag;
    std::call_once(sobol_extent_flag, [this]()
    { SobolSampler::SetSampleExtent({...}); });
    thread_local SobolSampler sobol;
    sobol.StartPixelSample(glm::ivec2(pixel_coord.x, pixel_coord.y), pixel_coord.z);
    auto ray = mCamera.GenerateRay(pixel_coord, sobol.Get2D());
*/
```

**问题分析**：
注释掉的代码看起来是正确的用于相机采样，但问题在于**后续路径追踪循环中混用了RNG和Sobol**。

### 问题2：路径追踪中的维度消耗不一致

在路径追踪中：
- 如果路径在第3次反弹终止，消耗了N个维度
- 如果路径在第10次反弹终止，消耗了M个维度（M > N）

当所有路径尝试使用同一个Sobol采样器时，不同像素的采样位置会不同步。

### 问题3：拒绝采样接口设计

`spherical.hpp`中注释掉的`UniformSampleSphere(const Sampler &sequence)`使用了do-while循环，会消耗不确定数量的维度。

### 问题4：Light::SampleLight的默认实现

```cpp
// light.hpp 第31-36行
virtual std::optional<LightInfo> SampleLight(..., const Sampler &sequence, ...) const
{
    thread_local RNG rng{};
    rng.SetSeed(static_cast<uint32_t>(sequence.GetSampleIndex()) ^ 0x9e3779b9u);
    return SampleLight(surface_point, scene_radius, rng, MISC);
}
```

**问题**：这个回退实现使用RNG来模拟Sampler，但种子只依赖于样本索引，不同像素会得到相同的随机序列。

---

## 修复建议与实现指导

### 建议1：分离采样策略

创建一个包含多个采样源的复合采样器：

```cpp
class PathSampler
{
public:
    // 用于关键采样点的低差异序列
    SobolSampler mSobol;
    
    // 用于决策点的独立随机数
    RNG mRNG;
    
    void StartPixelSample(const glm::ivec2 &pixel, int sample_index)
    {
        mSobol.StartPixelSample(pixel, sample_index);
        mRNG.SetSeed(pixel.x + pixel.y * 10000 + sample_index * 10000 * 10000);
    }
    
    // 相机采样（低差异序列）
    glm::vec2 GetCameraSample() { return mSobol.Get2D(); }
    
    // BSDF采样（可选择使用低差异或随机）
    glm::vec2 GetBSDFSample(int bounce) { return mSobol.Get2D(); }
    
    // 光源选择（低差异序列）
    float GetLightSelectSample() { return mSobol.Get1D(); }
    
    // 光源表面采样（低差异序列）
    glm::vec2 GetLightSurfaceSample() { return mSobol.Get2D(); }
    
    // 俄罗斯轮盘赌（独立随机数）
    float GetRussianRouletteSample() { return mRNG.Uniform(); }
    
    // 反射/透射决策（独立随机数）
    float GetDecisionSample() { return mRNG.Uniform(); }
};
```

### 建议2：修复球面均匀采样

**修改** `spherical.hpp`，移除拒绝采样版本，添加参数化版本：

```cpp
// 使用2D输入的参数化球面采样（固定2个维度）
inline glm::vec3 UniformSampleSphere(const glm::vec2 &u)
{
    float z = 1.f - 2.f * u.x;
    float r = glm::sqrt(glm::max(0.f, 1.f - z * z));
    float phi = 2.f * PI * u.y;
    return glm::vec3(r * glm::cos(phi), z, r * glm::sin(phi));
}
```

### 建议3：修复Light的Sampler接口

```cpp
// 为每个Light子类实现正确的Sampler版本
std::optional<LightInfo> AreaLight::SampleLight(..., const Sampler &sequence, ...) const
{
    auto shape_sample = mShape.SampleShape(sequence.Get2D()); // 使用2D采样
    // ...
}
```

### 建议4：维度预算管理

为路径追踪定义固定的维度预算：

```cpp
// 维度分配常量
constexpr int DIM_CAMERA_U = 0;
constexpr int DIM_CAMERA_V = 1;
constexpr int DIM_LIGHT_SELECT = 2;
constexpr int DIM_LIGHT_U = 3;
constexpr int DIM_LIGHT_V = 4;

// 每次反弹的维度数
constexpr int DIMS_PER_BOUNCE = 5;  // BSDF(2) + Light(3)

// 获取特定反弹的维度
int GetBounceStartDimension(int bounce)
{
    return 5 + bounce * DIMS_PER_BOUNCE;  // 跳过相机和第一次光源采样
}
```

---

## 接口设计建议

### 当前接口评估

| 接口 | 位置 | 状态 | 建议 |
|------|------|------|------|
| `Material::SampleBSDF(RNG&)` | material.hpp | ✅ 正确 | 保留 |
| `Material::SampleBSDF(Sampler&)` | material.hpp | ⚠️ 可改进 | 改为Get2D()输入 |
| `Microfacet::SampleVisibleNormal(RNG&)` | microfacet.hpp | ✅ 正确 | 保留 |
| `Microfacet::SampleVisibleNormal(Sampler&)` | microfacet.hpp | ✅ 正确 | 保留 |
| `Light::SampleLight(RNG&)` | light.hpp | ✅ 正确 | 保留 |
| `Light::SampleLight(Sampler&)` | light.hpp | ⚠️ 需改进 | 移除默认实现 |
| `Shape::SampleShape(RNG&)` | shape.hpp | ✅ 正确 | 保留 |
| `Shape::SampleShape(Sampler&)` | shape.hpp | ❌ 已注释 | 改为Get2D()输入 |
| `UniformSampleSphere(RNG&)` | spherical.hpp | ⚠️ 拒绝采样 | 添加参数化版本 |

### 推荐的接口改进

将`Sampler`参数改为显式的`glm::vec2`或`float`输入，可以：
1. 明确维度消耗
2. 支持不同的采样策略
3. 允许预计算采样点

```cpp
// 改进前
std::optional<BSDFInfo> SampleBSDF(const glm::vec3 &hit_point, const glm::vec3 &view_dir, const Sampler &sequence) const;

// 改进后
std::optional<BSDFInfo> SampleBSDF(const glm::vec3 &hit_point, const glm::vec3 &view_dir, const glm::vec2 &u) const;
```

---

## 最佳实践总结

1. **相机采样**：✅ 使用Sobol序列
2. **BSDF方向采样**：✅ 可以使用Sobol序列（注意维度管理）
3. **光源选择**：✅ 使用Sobol序列的1D采样
4. **光源表面采样**：✅ 使用Sobol序列的2D采样
5. **俄罗斯轮盘赌**：❌ 使用独立RNG
6. **反射/透射决策**：❌ 使用独立RNG
7. **拒绝采样**：❌ 避免使用，改用参数化方法

---

## 参考资料

- PBRT-v4: https://github.com/mmp/pbrt-v4
- "Efficient Rendering of Local Subsurface Scattering" - Matt Pharr, 论文
- "Physically Based Rendering: From Theory to Implementation" - 第4版，第8章
