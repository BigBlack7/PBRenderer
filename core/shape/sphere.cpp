#include "sphere.hpp"
#include "sampler/spherical.hpp"

namespace pbrt
{
    std::optional<HitInfo> Sphere::Intersect(const Ray &ray, float t_min, float t_max) const
    {
        glm::vec3 center_to_ray = ray.__origin__ - __center__;
        float a = glm::dot(ray.__direction__, ray.__direction__);
        float b = 2.f * glm::dot(center_to_ray, ray.__direction__);
        float c = glm::dot(center_to_ray, center_to_ray) - __radius__ * __radius__;
        float discriminant = b * b - 4.f * a * c;
        if (discriminant < 0.f)
        {
            return {};
        }

        float hit_t = (-b - glm::sqrt(discriminant)) * 0.5f / a;
        if (hit_t <= t_min)
        {
            hit_t = (-b + glm::sqrt(discriminant)) * 0.5f / a;
        }
        if (hit_t > t_min && hit_t < t_max)
        {
            glm::vec3 hit_point = ray.Hit(hit_t);
            glm::vec3 normal = glm::normalize(hit_point - __center__);
            return HitInfo{hit_t, hit_point, normal};
        }
        return {};
    }

    float Sphere::GetArea() const
    {
        return 4.f * PI * __radius__ * __radius__;
    }

    std::optional<ShapeInfo> Sphere::SampleShape(const RNG &rng) const
    {
        glm::vec3 normal = UniformSampleSphere(rng);
        return ShapeInfo{__center__ + __radius__ * normal, normal, 1.f / GetArea()};
    }
}