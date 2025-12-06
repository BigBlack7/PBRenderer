#pragma once
#include "shape.hpp"

namespace pt
{
    struct ShapeInfo
    {
        const Shape &__shape__;
        Material __material__;
        glm::mat4 __worldFromObject__;
        glm::mat4 __objectFromWorld__;
    };

    struct Scene : public Shape
    {
    private:
        std::vector<ShapeInfo> __shapeInfos__;

    public:
        void AddShape(
            const Shape &shape,
            const Material &material = {},
            const glm::vec3 &position = {0.f, 0.f, 0.f},
            const glm::vec3 &scale = {1.f, 1.f, 1.f},
            const glm::vec3 &rotate = {0.f, 0.f, 0.f});

        std::optional<HitInfo> Intersect(
            const Ray &ray,
            float t_min = 1e-5,
            float t_max = std::numeric_limits<float>::infinity()) const override;
    };
}