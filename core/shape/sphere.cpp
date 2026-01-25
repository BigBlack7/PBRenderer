#include "sphere.hpp"
#include "sampler/spherical.hpp"

namespace pbrt
{
    std::optional<HitInfo> Sphere::Intersect(const Ray &ray, float t_min, float t_max) const
    {
        /*
            球体方程: (x - Cₓ)² + (y - Cᵧ)² + (z - C₂)² = r²
            射线方程: P = O + tD

            (O + tD - C)² = r²
            (tD + (O - C))² = r²
            t²(D·D) + 2t(D·(O-C)) + (O-C)·(O-C) - r² = 0

            at² + bt + c = 0
            a = D·D: 射线方向的平方（通常等于1, 如果是单位向量）
            b = 2(D·(O-C)): 射线方向与球心-射线原点向量的点积的两倍
            c = |O-C|² - r²: 球心到射线原点距离的平方减去半径平方

            判别式Δ = b² - 4ac决定了方程解的性质：
            Δ < 0: 无实数解, 射线不与球体相交
            Δ = 0: 一个实数解, 射线与球体相切
            Δ > 0: 两个实数解, 射线穿过球体

            t₁ = (-b - √Δ) / (2a): 射线进入球体的点
            t₂ = (-b + √Δ) / (2a): 射线离开球体的点
        */
        glm::vec3 center_to_ray = ray.__origin__ - __center__;
        float a = glm::dot(ray.__direction__, ray.__direction__);
        float b = 2.f * glm::dot(center_to_ray, ray.__direction__);
        float c = glm::dot(center_to_ray, center_to_ray) - __radius__ * __radius__;

        float discriminant = b * b - 4.f * a * c;
        if (discriminant < 0.f)
        {
            return std::nullopt;
        }

        // 优先选择较小的t值(进入点), 如果进入点在有效范围外, 选择离开点, 确保总是返回最近的合法交点
        float hit_t = (-b - glm::sqrt(discriminant)) * 0.5f / a;
        if (hit_t <= t_min)
        {
            hit_t = (-b + glm::sqrt(discriminant)) * 0.5f / a;
        }

        if (hit_t > t_min && hit_t < t_max)
        {
            glm::vec3 hit_point = ray.Hit(hit_t);
            glm::vec3 normal = glm::normalize(hit_point - __center__);
            return HitInfo{
                .__t__ = hit_t,
                .__hitPoint__ = hit_point,
                .__normal__ = normal
                // end
            };
        }
        return std::nullopt;
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