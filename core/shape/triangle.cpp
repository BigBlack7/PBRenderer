#include "triangle.hpp"

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
}