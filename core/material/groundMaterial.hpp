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
        std::optional<BSDFSample> SampleBSDF(const glm::vec3 &hit_point, const glm::vec3 &view_dir, const RNG &rng) const override;
    };
}