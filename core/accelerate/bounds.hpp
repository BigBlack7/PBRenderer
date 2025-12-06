#pragma once
#include "presentation/ray.hpp"

namespace pt
{
    struct Bounds
    {
    public:
        glm::vec3 __bMin__;
        glm::vec3 __bMax__;

    public:
        Bounds() : __bMin__(std::numeric_limits<float>::max()), __bMax__(-std::numeric_limits<float>::max()) {}
        Bounds(const glm::vec3 &b_min, const glm::vec3 &b_max) : __bMin__(b_min), __bMax__(b_max) {}

        void Expand(const glm::vec3 &position)
        {
            __bMin__ = glm::min(__bMin__, position);
            __bMax__ = glm::max(__bMax__, position);
        }

        void Expand(const Bounds &bounds)
        {
            __bMin__ = glm::min(__bMin__, bounds.__bMin__);
            __bMax__ = glm::max(__bMax__, bounds.__bMax__);
        }

        bool HasIntersection(const Ray &ray, float t_min, float t_max) const;
        bool HasIntersection(const Ray &ray, const glm::vec3 &inv_dir, float t_min, float t_max) const;
        glm::vec3 GetDiagonal() const { return __bMax__ - __bMin__; }
        float GetSurfaceArea() const 
        {
            auto diagonal = GetDiagonal();
            return (diagonal.x * (diagonal.y + diagonal.z) + diagonal.y * diagonal.z) * 2.f;
        }
    };
}