#pragma once
#include "shape.hpp"

namespace pbrt
{
    struct Triangle : public Shape
    {
    public:
        glm::vec3 __p0__, __p1__, __p2__;
        glm::vec3 __n0__, __n1__, __n2__;

    public:
        Triangle(const glm::vec3 &p0, const glm::vec3 &p1, const glm::vec3 &p2,
                 const glm::vec3 &n0, const glm::vec3 &n1, const glm::vec3 &n2)
            : __p0__(p0), __p1__(p1), __p2__(p2), __n0__(n0), __n1__(n1), __n2__(n2) {}

        Triangle(const glm::vec3 &p0, const glm::vec3 &p1, const glm::vec3 &p2)
            : __p0__(p0), __p1__(p1), __p2__(p2)
        {
            glm::vec3 edge1 = __p1__ - __p0__;
            glm::vec3 edge2 = __p2__ - __p0__;
            __n0__ = glm::normalize(glm::cross(edge1, edge2));
            __n1__ = __n0__;
            __n2__ = __n0__;
        }

        std::optional<HitInfo> Intersect(const Ray &ray, float t_min, float t_max) const override;
        Bounds GetBounds() const override
        {
            Bounds bounds{};
            bounds.Expand(__p0__);
            bounds.Expand(__p1__);
            bounds.Expand(__p2__);
            return bounds;
        }

        float GetArea() const override;
        std::optional<ShapeInfo> SampleShape(const RNG &rng) const override;
    };
}