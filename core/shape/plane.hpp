#pragma once
#include "shape.hpp"

namespace pbrt
{
    struct Plane : public Shape
    {
    public:
        glm::vec3 __point__;
        glm::vec3 __normal__;

    public:
        Plane(const glm::vec3 &point, const glm::vec3 &normal) : __point__(point), __normal__(glm::normalize(normal)) {}

        std::optional<HitInfo> Intersect(const Ray &ray, float t_min, float t_max) const override;
    };
}