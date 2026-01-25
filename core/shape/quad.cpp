#include "quad.hpp"

namespace pbrt
{
    std::optional<HitInfo> Quad::Intersect(const Ray &ray, float t_min, float t_max) const
    {
        /*
            平面方程: (P - P₀) · N = 0
            P₀: 平面上的一点(四边形中心)
            N: 平面法线
            P: 平面上任意点

            射线方程: P = O + tD

            (O + tD - P₀) · N = 0
            t = (P₀ - O) · N / (D · N)
        */
        float denominator = glm::dot(ray.__direction__, __normal__);

        // 避免除零 - 射线与平面平行
        if (glm::abs(denominator) < 1e-6f)
        {
            return std::nullopt;
        }

        float hit_t = glm::dot(__point__ - ray.__origin__, __normal__) / denominator;

        // 检查t值是否在有效范围内
        if (hit_t <= t_min || hit_t >= t_max)
        {
            return std::nullopt;
        }

        // 计算交点
        glm::vec3 hit_point = ray.Hit(hit_t);

        // 将交点转换到局部坐标系
        glm::vec3 hit_point_to_center = hit_point - __point__;
        float x_local = glm::dot(hit_point_to_center, __xAxis__);
        float z_local = glm::dot(hit_point_to_center, __zAxis__);

        // 检查交点是否在四边形范围内 - 现在支持不同的x和z范围
        if (glm::abs(x_local) <= __halfWidthX__ && glm::abs(z_local) <= __halfWidthZ__)
        {
            return HitInfo{
                .__t__ = hit_t,
                .__hitPoint__ = hit_point,
                .__normal__ = __normal__
                // end
            };
        }

        return std::nullopt;
    }

    float Quad::GetArea() const
    {
        return 4.f * __halfWidthX__ * __halfWidthZ__; // (2 * halfWidthX) * (2 * halfWidthZ)
    }

    std::optional<ShapeInfo> Quad::SampleShape(const RNG &rng) const
    {
        // 在矩形范围内均匀采样 - 支持不同的x和z范围
        float x = (rng.Uniform() * 2.f - 1.f) * __halfWidthX__;
        float z = (rng.Uniform() * 2.f - 1.f) * __halfWidthZ__;

        glm::vec3 sample_point = __point__ + x * __xAxis__ + z * __zAxis__;
        return ShapeInfo{sample_point, __normal__, 1.f / GetArea()};
    }
}