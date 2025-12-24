#pragma once
#include "material.hpp"

namespace pbrt
{
    class GroundMaterial : public Material
    {
    private:
        glm::vec3 mAlbedo{};

    public:
        GroundMaterial(const glm::vec3 &albedo) : mAlbedo(albedo) {}
        std::optional<BSDFInfo> SampleBSDF(const glm::vec3 &hit_point, const glm::vec3 &view_dir, const RNG &rng) const override;
        glm::vec3 BSDF(const glm::vec3 &hit_point, const glm::vec3 &light_dir, const glm::vec3 &view_dir) const override;
        float PDF(const glm::vec3 &hit_point, const glm::vec3 &light_dir, const glm::vec3 &view_dir) const override;
        bool IsDeltaDistribution() const override { return false; }
    };
}