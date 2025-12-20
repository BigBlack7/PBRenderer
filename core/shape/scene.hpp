#pragma once
#include "accelerate/sceneBVH.hpp"
#include "light/areaLight.hpp"
#include "light/infiniteLight.hpp"
#include "sampler/lightSampler.hpp"

namespace pbrt
{
    struct Scene : public Shape
    {
    private:
        std::vector<ShapeBVHInfo> __shapeInfos__;
        SceneBVH __sceneBVH__;
        LightSampler __lightSampler__;
        std::vector<const InfiniteLight *> __infiniteLights__;
        float __radius__;

    public:
        void AddShape(
            const Shape &shape,
            const Material *material = nullptr,
            const glm::vec3 &position = {0.f, 0.f, 0.f},
            const glm::vec3 &scale = {1.f, 1.f, 1.f},
            const glm::vec3 &rotate = {0.f, 0.f, 0.f});

        void AddAreaLight(const AreaLight *light, Material *material)
        {
            material->mAreaLight = light;
            AddShape(light->GetShape(), material);
            __lightSampler__.AddLight(light);
        }

        void AddInfiniteLight(const InfiniteLight *light)
        {
            __lightSampler__.AddLight(light);
            __infiniteLights__.push_back(light);
        }

        std::optional<HitInfo> Intersect(
            const Ray &ray,
            float t_min = 1e-5,
            float t_max = std::numeric_limits<float>::infinity()) const override;

        void Build()
        {
            __sceneBVH__.Build(std::move(__shapeInfos__));
            auto scene_bounds = __sceneBVH__.GetBounds();
            __radius__ = 0.5f * glm::distance(scene_bounds.__bMax__, scene_bounds.__bMin__);
            __lightSampler__.Build(__radius__);
        }

        const LightSampler &GetLightSampler() const { return __lightSampler__; }

        float GetRadius() const { return __radius__; } 

        const std::vector<const InfiniteLight *> &GetInfiniteLights() const { return __infiniteLights__; }
    };
}