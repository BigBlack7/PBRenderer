#pragma once
#include "light/ray.hpp"
#include "accelerate/bounds.hpp"
#include "sequence/sampler.hpp"
#include <optional>

namespace pbrt
{
    struct ShapeInfo
    {
        glm::vec3 __point__;
        glm::vec3 __normal__;
        float __pdf__;
    };

    struct Shape
    {
    public:
        virtual std::optional<HitInfo> Intersect(const Ray &ray, float t_min, float t_max) const = 0;
        virtual Bounds GetBounds() const { return {}; }
        virtual float GetArea() const { return -1.f; }
        virtual std::optional<ShapeInfo> SampleShape(const RNG &rng) const { return {}; }
        // 参数化采样接口：使用2D均匀随机数作为输入，适用于低差异序列
        virtual std::optional<ShapeInfo> SampleShape(const glm::vec2 &u) const { return {}; }
        // 参数化采样接口：用于复合形状（如BVH/Model），需要1D选择 + 2D表面采样
        // 默认实现忽略u_select，仅适用于简单形状；复合形状必须重写此方法
        virtual std::optional<ShapeInfo> SampleShape(float u_select, const glm::vec2 &u_surface) const { return SampleShape(u_surface); }
        virtual float PDF(const glm::vec3 &point, const glm::vec3 &normal) const { return 1.f / GetArea(); }
    };
}