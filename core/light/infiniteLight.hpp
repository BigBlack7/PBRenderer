#pragma once
#include "light.hpp"

namespace pbrt
{
    class InfiniteLight : public Light
    {
    public:
        InfiniteLight(const glm::vec3 &Le) : Light(Le) {}

        float Phi(float scene_radius) const override;
        std::optional<LightInfo> SampleLight(const glm::vec3 &surface_point, float scene_radius, const RNG &rng) const override;
    };
}