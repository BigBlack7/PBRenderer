#pragma once
#include "material.hpp"
#include "microfacet.hpp"

namespace pbrt
{
    class ConductorMaterial : public Material
    {
    private:
        glm::vec3 mIOR, mK;
        Microfacet mMicrofacet;

    private:
        glm::vec3 Fresnel(const glm::vec3 &ior, const glm::vec3 &k, float cos_theta_i) const;

    public:
        ConductorMaterial(const glm::vec3 &ior, const glm::vec3 &k, float alpha_x, float alpha_z) : mIOR(ior), mK(k), mMicrofacet(alpha_x, alpha_z) {}
        std::optional<BSDFInfo> SampleBSDF(const glm::vec3 &hit_point, const glm::vec3 &view_dir, const RNG &rng) const override;
        glm::vec3 BSDF(const glm::vec3 &hit_point, const glm::vec3 &light_dir, const glm::vec3 &view_dir) const override;
        float PDF(const glm::vec3 &hit_point, const glm::vec3 &light_dir, const glm::vec3 &view_dir) const override;
        bool IsDeltaDistribution() const override { return mMicrofacet.IsDeltaDistribution(); }
    };
}