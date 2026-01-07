#pragma once
#include "light.hpp"

namespace pbrt
{
    class InfiniteLight : public Light
    {
    private:
        glm::vec3 mLe;

    public:
        InfiniteLight(const glm::vec3 &Le) : mLe(Le) {}

        LightType GetLightType() const override { return LightType::Infinite; }
        float Phi(float scene_radius) const override;
        std::optional<LightInfo> SampleLight(const glm::vec3 &surface_point, float scene_radius, const RNG &rng, bool MISC) const override;
        // std::optional<LightInfo> SampleLight(const glm::vec3 &surface_point, float scene_radius, const Sampler &sequence, bool MISC) const override;
        float PDF(const glm::vec3 &surface_point, const glm::vec3 &light_point, const glm::vec3 &normal, bool MISC) const override;
        glm::vec3 GetRadiance(const glm::vec3 &surface_point, const glm::vec3 &light_point, const glm::vec3 &normal) const override;
    };
}