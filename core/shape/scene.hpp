#pragma once
#include "accelerate/sceneBVH.hpp"

namespace pbrt
{
    struct Scene : public Shape
    {
    private:
        std::vector<ShapeInfo> __shapeInfos__;
        SceneBVH __sceneBVH__;

    public:
        void AddShape(
            const Shape &shape,
            const Material *material = nullptr,
            const glm::vec3 &position = {0.f, 0.f, 0.f},
            const glm::vec3 &scale = {1.f, 1.f, 1.f},
            const glm::vec3 &rotate = {0.f, 0.f, 0.f});

        std::optional<HitInfo> Intersect(
            const Ray &ray,
            float t_min = 1e-5,
            float t_max = std::numeric_limits<float>::infinity()) const override;

        void Build() { __sceneBVH__.Build(std::move(__shapeInfos__)); }
    };
}