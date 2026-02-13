#include "triangle.hpp"
#include "sampler/spherical.hpp"

namespace pbrt
{
    std::optional<HitInfo> Triangle::Intersect(const Ray &ray, float t_min, float t_max) const
    {
        /*
            射线方程P = O + tD
            O: 射线原点
            D: 射线方向
            t: 交点参数, t>0表示在射线正方向上

            P = P₀ + u(P₁ - P₀) + v(P₂ - P₀) = P₀ + ue₁ + ve₂ 其中u, v为重心坐标参数, u ≥ 0, v ≥ 0, u + v ≤ 1

            O + tD = P₀ + ue₁ + ve₂
            O - P₀ = -tD + ue₁ + ve₂
            S = O - P₀

            [-D e₁ e₂][t u v]ᵀ = S
            S₁ = D × e₂
            S₂ = S × e₁

            [t u v]ᵀ = 1/( S₁ · e₁)[S₂ · e₂, S₁ · S, S₂ · D]ᵀ
        */
        glm::vec3 e1 = __p1__ - __p0__; // 三角形边
        glm::vec3 e2 = __p2__ - __p0__;
        glm::vec3 s1 = glm::cross(ray.__direction__, e2);
        float inv_det = 1.f / glm::dot(e1, s1);

        glm::vec3 s = ray.__origin__ - __p0__;
        float u = glm::dot(s, s1) * inv_det;
        if (u < 0.f || u > 1.f)
        {
            return std::nullopt;
        }

        glm::vec3 s2 = glm::cross(s, e1);
        float v = glm::dot(ray.__direction__, s2) * inv_det;
        if (v < 0.f || u + v > 1.f)
        {
            return std::nullopt;
        }

        float t = glm::dot(e2, s2) * inv_det;
        if (t > t_min && t < t_max)
        {
            glm::vec3 hit_point = ray.Hit(t);
            glm::vec3 normal = (1.f - u - v) * __n0__ + u * __n1__ + v * __n2__;
            return HitInfo{
                .__t__ = t,
                .__hitPoint__ = hit_point,
                .__normal__ = glm::normalize(normal)
                // end
            };
        }
        return std::nullopt;
    }

    float Triangle::GetArea() const
    {
        return glm::length(glm::cross(__p2__ - __p0__, __p1__ - __p0__)) * 0.5f;
    }

    std::optional<ShapeInfo> Triangle::SampleShape(const RNG &rng) const
    {
        float u = rng.Uniform(), v = rng.Uniform();
        if (u > v)
        {
            v *= 0.5f;
            u -= v;
        }
        else
        {
            u *= 0.5f;
            v -= u;
        }
        return ShapeInfo{
            .__point__ = u * __p0__ + v * __p1__ + (1.f - u - v) * __p2__,
            .__normal__ = u * __n0__ + v * __n1__ + (1.f - u - v) * __n2__,
            .__pdf__ = 1.f / GetArea()
            // end
        };
    }
}