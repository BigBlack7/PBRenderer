#pragma once
#include "material.hpp"
#include "microfacet.hpp"

namespace pbrt
{
    /*
        By Laurent Belcour, Pascal Barla, 2017.
        A Practical Extension to Microfacet Theory for the Modeling of Varying Iridescence
    */
    class IridescentMaterial : public Material
    {
    private:
        float mDinc;   // 薄膜厚度(微米)  0.f - 10.f 0.5f
        float mEta2;   // 薄膜折射率      1.f - 5.f  2.f
        float mEta3;   // 基底折射率      1.f - 5.f  3.f
        float mKappa3; // 基底消光系数    0.f - 5.f  0.f
        Microfacet mMicrofacet;

        // 电介质漫反射基底
        glm::vec3 mBaseColor{};

    private:
        void FresnelDielectric(float cos_theta1, float n1, float n2, glm::vec2 &R, glm::vec2 &phi) const;

        void FresnelConductor(float cos_theta1, float n1, float n2, float k, glm::vec2 &R, glm::vec2 &phi) const;

        // 评估薄膜层的XYZ敏感度曲线(用于薄膜干涉)
        glm::vec3 EvalSensitivity(float opd, float shift) const;

        // 计算基于薄膜干涉的虹彩颜色
        glm::vec3 ComputeIridescence(float cos_theta1, float cos_theta2) const;

    public:
        IridescentMaterial(float dinc, float eta2, float eta3, float kappa3, float alpha_x, float alpha_z, const glm::vec3 &base_color = glm::vec3(1.0f))
            : mDinc(dinc), mEta2(eta2), mEta3(eta3), mKappa3(kappa3), mMicrofacet(alpha_x, alpha_z), mBaseColor(base_color) {}

        std::optional<BSDFInfo> SampleBSDF(const glm::vec3 &hit_point, const glm::vec3 &view_dir, const RNG &rng) const override;
        std::optional<BSDFInfo> SampleBSDF(const glm::vec3 &hit_point, const glm::vec3 &view_dir, const Sampler &sequence) const override;
        glm::vec3 BSDF(const glm::vec3 &hit_point, const glm::vec3 &light_dir, const glm::vec3 &view_dir) const override;
        float PDF(const glm::vec3 &hit_point, const glm::vec3 &light_dir, const glm::vec3 &view_dir) const override;
        bool IsDeltaDistribution() const override { return mMicrofacet.IsDeltaDistribution(); }
        void Regularize() const override;
    };
}