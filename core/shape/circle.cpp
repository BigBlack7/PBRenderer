#include "circle.hpp"
#include "sampler/spherical.hpp"

namespace pbrt
{
    std::optional<HitInfo> Circle::Intersect(const Ray &ray, float t_min, float t_max) const
    {
        /*
            平面方程: (P - P₀) · N = 0
            P₀: 平面上的一点(圆心)
            N: 平面法线
            P: 平面上任意点

            射线方程: P = O + tD

            (O + tD - P₀) · N = 0
            t = (P₀ - O) · N / (D · N)
        */
        float hit_t = glm::dot(__point__ - ray.__origin__, __normal__) / glm::dot(ray.__direction__, __normal__);
        glm::vec3 hit_point_to_center = ray.Hit(hit_t) - __point__;
        if ((hit_t > t_min && hit_t < t_max) && (glm::dot(hit_point_to_center, hit_point_to_center) < __radius__ * __radius__))
        {
            return HitInfo{
                .__t__ = hit_t,
                .__hitPoint__ = ray.Hit(hit_t),
                .__normal__ = __normal__
                // end
            };
        }
        return std::nullopt;
    }

    float Circle::GetArea() const
    {
        return PI * __radius__ * __radius__;
    }

    std::optional<ShapeInfo> Circle::SampleShape(const RNG &rng) const
    {
        glm::vec2 sample_local = UniformSampleUnitDisk({rng.Uniform(), rng.Uniform()}) * __radius__;  // 局部坐标系下的采样点
        glm::vec3 sample_point = __point__ + sample_local.x * __xAxis__ + sample_local.y * __zAxis__; // 世界坐标系下的采样点
        return ShapeInfo{
            .__point__ = sample_point,
            .__normal__ = __normal__,
            .__pdf__ = 1.f / GetArea()
            // end
        };
    }
}