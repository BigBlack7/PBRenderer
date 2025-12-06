#pragma once
#include "presentation/ray.hpp"
#include <optional>

namespace pt
{
    struct Shape
    {
    public:
        virtual std::optional<HitInfo> Intersect(const Ray &ray, float t_min, float t_max) const = 0;
    };
}