#pragma once
#include "material.hpp"

namespace pbrt
{
    class SpecularMaterial : public Material
    {
    private:
        glm::vec3 mAlbedo{};

    public:
        SpecularMaterial(const glm::vec3 &albedo) : mAlbedo(albedo) {}
        std::optional<BSDFInfo> SampleBSDF(const glm::vec3 &hit_point, const glm::vec3 &view_dir, const RNG &rng) const override;
        glm::vec3 BSDF(const glm::vec3 &hit_point, const glm::vec3 &light_dir, const glm::vec3 &view_dir) const override { return {}; }
        bool IsDeltaDistribution() const override { return true; }
    };
}