#include "ray.hpp"

namespace pbrt
{
    // 将射线从世界空间变换到物体空间
    Ray Ray::ObjectFromWorld(const glm::mat4 &object_from_world) const
    {
        glm::vec3 o = object_from_world * glm::vec4(__origin__, 1.f);
        glm::vec3 d = object_from_world * glm::vec4(__direction__, 0.f);
        return Ray{o, d};
    }
}