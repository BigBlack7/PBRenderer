#pragma once
#include "material.hpp"
#include "microfacet.hpp"

namespace pbrt
{
    class DielectricMaterial : public Material
    {
    private:
        glm::vec3 mAlbedoR{};
        glm::vec3 mAlbedoT{};
        float mIOR{};
        Microfacet mMicrofacet;

    private:
        float Fresnel(float etai_div_etat, float cos_theta_t, float &cos_theta_i) const;

    public:
        DielectricMaterial(const glm::vec3 &albedo, float ior, float alpha_x, float alpha_z) : mAlbedoR(albedo), mAlbedoT(albedo), mIOR(ior), mMicrofacet(alpha_x, alpha_z) {}
        DielectricMaterial(const glm::vec3 &albedo_r, const glm::vec3 &albedo_t, float ior, float alpha_x, float alpha_z) : mAlbedoR(albedo_r), mAlbedoT(albedo_t), mIOR(ior), mMicrofacet(alpha_x, alpha_z) {}
        std::optional<BSDFInfo> SampleBSDF(const glm::vec3 &hit_point, const glm::vec3 &view_dir, const RNG &rng) const override;
        glm::vec3 BSDF(const glm::vec3 &hit_point, const glm::vec3 &light_dir, const glm::vec3 &view_dir) const override;
        bool IsDeltaDistribution() const override { return mMicrofacet.IsDeltaDistribution() || (mIOR == 1.f); }
    };
}