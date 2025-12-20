#include "plane.hpp"
#include "sampler/spherical.hpp"

namespace pbrt
{
    std::optional<HitInfo> pbrt::Plane::Intersect(const Ray &ray, float t_min, float t_max) const
    {
        float hit_t = glm::dot(__point__ - ray.__origin__, __normal__) / glm::dot(ray.__direction__, __normal__);
        glm::vec3 hit_point_to_center = ray.Hit(hit_t) - __point__;
        if ((hit_t > t_min && hit_t < t_max) && (glm::dot(hit_point_to_center, hit_point_to_center) < __radius__ * __radius__))
        {
            return HitInfo{hit_t, ray.Hit(hit_t), __normal__};
        }
        return {};
    }

    float Plane::GetArea() const
    {
        return PI * __radius__ * __radius__;
    }

    std::optional<ShapeInfo> Plane::SampleShape(const RNG &rng) const
    {
        glm::vec2 sample_local = UniformSampleUnitDisk({rng.Uniform(), rng.Uniform()}) * __radius__;
        glm::vec3 sample_point = __point__ + sample_local.x * __xAxis__ + sample_local.y * __zAxis__;
        return ShapeInfo{sample_point, __normal__, 1.f / GetArea()};
    }
}