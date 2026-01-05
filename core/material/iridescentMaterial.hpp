#pragma once
#include "material.hpp"
#include "microfacet.hpp"

namespace pbrt
{
    class IridescentMaterial : public Material
    {
    private:
        float mDinc;        // Thin film thickness (in micrometers)
        float mEta2;        // IOR of thin film layer
        float mEta3;        // IOR of base material
        float mKappa3;      // Extinction coefficient of base material
        glm::vec3 mBaseColor; // Base diffuse color (for dielectric substrates when kappa3 = 0)
        Microfacet mMicrofacet;

    private:
        // Fresnel equations for dielectric/dielectric interfaces
        void FresnelDielectric(float cos_theta1, float n1, float n2, glm::vec2 &R, glm::vec2 &phi) const;
        
        // Fresnel equations for dielectric/conductor interfaces
        void FresnelConductor(float cos_theta1, float n1, float n2, float k, glm::vec2 &R, glm::vec2 &phi) const;
        
        // Evaluate XYZ sensitivity curves in Fourier space (for thin-film interference)
        glm::vec3 EvalSensitivity(float opd, float shift) const;
        
        // Compute the iridescent color based on thin-film interference
        glm::vec3 ComputeIridescence(float cos_theta1, float cos_theta2) const;
        
        // Get effective eta2 with smooth interpolation to 1.0 when Dinc -> 0
        float GetEffectiveEta2() const;

    public:
        // Constructor with base color (for dielectric substrates)
        IridescentMaterial(float Dinc, float eta2, float eta3, float kappa3, float alpha_x, float alpha_z, 
                          const glm::vec3& base_color = glm::vec3(1.0f))
            : mDinc(Dinc), mEta2(eta2), mEta3(eta3), mKappa3(kappa3), mBaseColor(base_color), mMicrofacet(alpha_x, alpha_z) {}
        
        std::optional<BSDFInfo> SampleBSDF(const glm::vec3 &hit_point, const glm::vec3 &view_dir, const RNG &rng) const override;
        glm::vec3 BSDF(const glm::vec3 &hit_point, const glm::vec3 &light_dir, const glm::vec3 &view_dir) const override;
        float PDF(const glm::vec3 &hit_point, const glm::vec3 &light_dir, const glm::vec3 &view_dir) const override;
        bool IsDeltaDistribution() const override { return mMicrofacet.IsDeltaDistribution(); }
        void Regularize() const override;
    };
}
