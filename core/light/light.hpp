#pragma once
#include "utils/rng.hpp"
#include "sequence/rngSampler.hpp"
#include <glm/glm.hpp>
#include <optional>

namespace pbrt
{
    struct LightInfo
    {
        glm::vec3 __lightPoint__;
        glm::vec3 __direction__;
        glm::vec3 __Le__;
        float __pdf__;
    };

    enum class LightType
    {
        Area,
        Infinite,
        Environment
    };

    class Light
    {
    public:
        virtual LightType GetLightType() const = 0;
        virtual float Phi(float scene_radius) const = 0; // 光源功率 radiant flux
        virtual std::optional<LightInfo> SampleLight(const glm::vec3 &surface_point, float scene_radius, const RNG &rng, bool MISC) const = 0;

        virtual std::optional<LightInfo> SampleLight(const glm::vec3 &surface_point, float scene_radius, const Sampler &sequence, bool MISC) const
        {
            thread_local RNG rng{};
            rng.SetSeed(static_cast<uint32_t>(sequence.GetSampleIndex()) ^ 0x9e3779b9u);
            return SampleLight(surface_point, scene_radius, rng, MISC);
        }

        virtual float PDF(const glm::vec3 &surface_point, const glm::vec3 &light_point, const glm::vec3 &normal, bool MISC) const = 0;

        virtual glm::vec3 GetRadiance(const glm::vec3 &surface_point, const glm::vec3 &light_point, const glm::vec3 &normal) const = 0;
    };
}