#include "triangle.hpp"
#include "sampler/spherical.hpp"

namespace pbrt
{
    std::optional<HitInfo> Triangle::Intersect(const Ray &ray, float t_min, float t_max) const
    {
        glm::vec3 e1 = __p1__ - __p0__;
        glm::vec3 e2 = __p2__ - __p0__;
        glm::vec3 s1 = glm::cross(ray.__direction__, e2);
        float inv_det = 1.f / glm::dot(e1, s1);

        glm::vec3 s = ray.__origin__ - __p0__;
        float u = glm::dot(s, s1) * inv_det;
        if (u < 0.f || u > 1.f)
        {
            return {};
        }

        glm::vec3 s2 = glm::cross(s, e1);
        float v = glm::dot(ray.__direction__, s2) * inv_det;
        if (v < 0.f || u + v > 1.f)
        {
            return {};
        }

        float t = glm::dot(e2, s2) * inv_det;
        if (t > t_min && t < t_max)
        {
            glm::vec3 hit_point = ray.Hit(t);
            glm::vec3 normal = (1.f - u - v) * __n0__ + u * __n1__ + v * __n2__;
            return HitInfo{t, hit_point, glm::normalize(normal)};
        }
        return {};
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
        return ShapeInfo{u * __p0__ + v * __p1__ + (1.f - u - v) * __p2__, u * __n0__ + v * __n1__ + (1.f - u - v) * __n2__, 1.f / GetArea()};
    }

    // 参数化采样：使用2D均匀随机数作为输入，适用于低差异序列
    std::optional<ShapeInfo> Triangle::SampleShape(const glm::vec2 &uv) const
    {
        float u = uv.x, v = uv.y;
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
        return ShapeInfo{u * __p0__ + v * __p1__ + (1.f - u - v) * __p2__, u * __n0__ + v * __n1__ + (1.f - u - v) * __n2__, 1.f / GetArea()};
    }
}