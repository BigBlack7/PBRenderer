#include "plane.hpp"

namespace pbrt
{
    std::optional<HitInfo> pbrt::Plane::Intersect(const Ray &ray, float t_min, float t_max) const
    {
        float hit_t = glm::dot(__point__ - ray.__origin__, __normal__) / glm::dot(ray.__direction__, __normal__);
        if (hit_t > t_min && hit_t < t_max)
        {
            return HitInfo{hit_t, ray.Hit(hit_t), __normal__};
        }
        return {};
    }

}