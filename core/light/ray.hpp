#pragma once
#include "material/material.hpp"
#include "utils/debugMacro.hpp"
#include <glm/glm.hpp>

namespace pbrt
{
    struct Ray
    {
    public:
        glm::vec3 __origin__;
        glm::vec3 __direction__;

    public:
        glm::vec3 Hit(float t) const
        {
            return __origin__ + t * __direction__;
        }

        Ray ObjectFromWorld(const glm::mat4 &object_from_world) const;

        DEBUG_INFO(mutable size_t __boundsTestCount__ = 0)
        DEBUG_INFO(mutable size_t __triangleTestCount__ = 0)
    };

    struct HitInfo
    {
    public:
        float __t__;
        glm::vec3 __hitPoint__;
        glm::vec3 __normal__;
        const Material *__material__{nullptr};
    };
}