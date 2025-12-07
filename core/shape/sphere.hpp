#pragma once

#include "shape.hpp"

namespace pt
{
    struct Sphere : public Shape
    {
    public:
        glm::vec3 __center__;
        float __radius__;

    public:
        Sphere(const glm::vec3 &center, float radius) : __center__(center), __radius__(radius) {}
        std::optional<HitInfo> Intersect(const Ray &ray, float t_min, float t_max) const override;
        Bounds GetBounds() const override { return {__center__ - __radius__, __center__ + __radius__}; }
    };

}