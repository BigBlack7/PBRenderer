#pragma once
#include "shape.hpp"

namespace pbrt
{
    struct Plane : public Shape
    {
    public:
        glm::vec3 __point__;
        glm::vec3 __normal__;
        glm::vec3 __xAxis__, __zAxis__;
        float __radius__;

        Bounds __bounds__;

    public:
        Plane(const glm::vec3 &point, const glm::vec3 &normal, float radius) : __point__(point), __normal__(glm::normalize(normal)), __radius__(radius), __bounds__()
        {
            glm::vec3 up = glm::abs(__normal__.y) < 0.99999 ? glm::vec3(0, 1, 0) : glm::vec3(0, 0, 1);
            __xAxis__ = glm::normalize(glm::cross(__normal__, up));
            __zAxis__ = glm::normalize(glm::cross(__xAxis__, __normal__));

            Bounds local_bounds{{-radius, -0.001f, -radius}, {radius, 0.001f, radius}};
            for (size_t i = 0; i < 8; i++)
            {
                glm::vec3 corner = local_bounds.GetCorner(i);
                __bounds__.Expand(__point__ + corner.x * __xAxis__ + corner.y * __normal__ + corner.z * __zAxis__);
            }
        }

        Bounds GetBounds() const override { return __bounds__; }

        std::optional<HitInfo> Intersect(const Ray &ray, float t_min, float t_max) const override;
        float GetArea() const override;
        std::optional<ShapeInfo> SampleShape(const RNG &rng) const override;
    };
}