#pragma once
#include "material.hpp"

namespace pbrt
{
    class ConductorMaterial : public Material
    {
    private:
        glm::vec3 mIOR, mK;

        private:
            glm::vec3 Fresnel(const glm::vec3 &ior, const glm::vec3 &k, const glm::vec3 &view_dir) const;

        public:
            ConductorMaterial(const glm::vec3 &ior, const glm::vec3 &k) : mIOR(ior), mK(k) {}
            std::optional<BSDFSample> SampleBSDF(const glm::vec3 &hit_point, const glm::vec3 &view_dir, const RNG &rng) const override;
    };
}