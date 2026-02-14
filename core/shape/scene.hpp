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
        std::vector<ShapeBVHInfo> __shapeBVHInfos__;
        SceneBVH __sceneBVH__;
        LightSampler __lightSampler__;
        LightSampler __lightSamplerMISC__;
        std::vector<const Light *> __infiniteLights__;
        float __radius__;
        glm::vec3 __center__{};

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
            __lightSamplerMISC__.AddLight(light);
        }

        void AddInfiniteLight(const Light *light)
        {
            __lightSampler__.AddLight(light);
            if (!light->Impossible())
            {
                __lightSamplerMISC__.AddLight(light);
            }
            __infiniteLights__.push_back(light);
        }

        std::optional<HitInfo> Intersect(
            const Ray &ray,
            float t_min = 1e-5,
            float t_max = std::numeric_limits<float>::infinity()) const override;

        void Build()
        {
            __sceneBVH__.Build(std::move(__shapeBVHInfos__));
            auto scene_bounds = __sceneBVH__.GetBounds();
            __center__ = 0.5f * (scene_bounds.__bMax__ + scene_bounds.__bMin__);
            __radius__ = 0.5f * glm::distance(scene_bounds.__bMax__, scene_bounds.__bMin__);
            __lightSampler__.Build(__radius__);
            __lightSamplerMISC__.Build(__radius__);
        }

        const LightSampler &GetLightSampler(bool MISC) const { return MISC ? __lightSamplerMISC__ : __lightSampler__; }

        float GetRadius() const { return __radius__; }
        glm::vec3 GetCenter() const { return __center__; }

        const std::vector<const Light *> &GetInfiniteLights() const { return __infiniteLights__; }
    };
}