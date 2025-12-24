#pragma once
#include "light.hpp"

namespace pbrt
{
    class InfiniteLight : public Light
    {
    public:
        InfiniteLight(const glm::vec3 &Le) : Light(Le) {}

        LightType GetLightType() const override { return LightType::Infinite; }
        float Phi(float scene_radius) const override;
        std::optional<LightInfo> SampleLight(const glm::vec3 &surface_point, float scene_radius, const RNG &rng, bool MISC) const override;
        float PDF(const glm::vec3 &surface_point, const glm::vec3 &light_point, const glm::vec3 &normal, bool MISC) const override;
    };
}