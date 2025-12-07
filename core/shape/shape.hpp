#pragma once
#include "presentation/ray.hpp"
#include "accelerate/bounds.hpp"
#include <optional>

namespace pbrt
{
    struct Shape
    {
    public:
        virtual std::optional<HitInfo> Intersect(const Ray &ray, float t_min, float t_max) const = 0;
        virtual Bounds GetBounds() const { return {}; }
    };
}