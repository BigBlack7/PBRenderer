#pragma once
#include "shape.hpp"

namespace pbrt
{
    struct Quad : public Shape
    {
    public:
        glm::vec3 __point__;            // 四边形中心点
        glm::vec3 __normal__;           // 平面法线
        glm::vec3 __xAxis__, __zAxis__; // 平面的两个切线轴
        float __halfWidthX__;           // X方向半边长
        float __halfWidthZ__;           // Z方向半边长

        Bounds __bounds__;

    public:
        // Rectangle
        Quad(const glm::vec3 &point, const glm::vec3 &normal, float halfWidthX, float halfWidthZ)
            : __point__(point), __normal__(glm::normalize(normal)), __halfWidthX__(halfWidthX), __halfWidthZ__(halfWidthZ), __bounds__()
        {
            // 构建局部坐标系
            glm::vec3 up = glm::abs(__normal__.y) < 0.99999f ? glm::vec3(0.f, 1.f, 0.f) : glm::vec3(0.f, 0.f, 1.f);
            __xAxis__ = glm::normalize(glm::cross(__normal__, up));
            __zAxis__ = glm::normalize(glm::cross(__xAxis__, __normal__));

            // 计算包围盒 - 矩形的范围 [-halfWidthX, halfWidthX] 在x方向, [-halfWidthZ, halfWidthZ] 在z方向
            Bounds local_bounds{{-halfWidthX, -0.001f, -halfWidthZ}, {halfWidthX, 0.001f, halfWidthZ}};
            for (size_t i = 0; i < 8; i++)
            {
                glm::vec3 corner = local_bounds.GetCorner(i);
                __bounds__.Expand(__point__ + corner.x * __xAxis__ + corner.y * __normal__ + corner.z * __zAxis__);
            }
        }

        // Square
        Quad(const glm::vec3 &point, const glm::vec3 &normal, float halfWidth) : Quad(point, normal, halfWidth, halfWidth) {}

        Bounds GetBounds() const override { return __bounds__; }

        std::optional<HitInfo> Intersect(const Ray &ray, float t_min, float t_max) const override;
        float GetArea() const override;
        std::optional<ShapeInfo> SampleShape(const RNG &rng) const override;
    };
}