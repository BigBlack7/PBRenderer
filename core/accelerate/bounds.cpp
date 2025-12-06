#include "bounds.hpp"

namespace pt
{
    bool Bounds::HasIntersection(const Ray &ray, float t_min, float t_max) const
    {
        glm::vec3 inv_dir = 1.f / ray.__direction__;
        glm::vec3 t1 = (__bMin__ - ray.__origin__) * inv_dir;
        glm::vec3 t2 = (__bMax__ - ray.__origin__) * inv_dir;
        glm::vec3 tmin = glm::min(t1, t2);
        glm::vec3 tmax = glm::max(t1, t2);
        float near = glm::max(tmin.x, glm::max(tmin.y, tmin.z));
        float far = glm::min(tmax.x, glm::min(tmax.y, tmax.z));
        return glm::max(near, t_min) <= glm::min(far, t_max);
    }
}