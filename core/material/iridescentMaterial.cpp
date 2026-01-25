#include "iridescentMaterial.hpp"
#include "sampler/spherical.hpp"
#include <cmath>

namespace pbrt
{
    // XYZ to CIE 1931 RGB color space (using neutral E illuminant)
    static const glm::mat3 XYZ_TO_RGB = glm::mat3(
        2.3706743, -0.5138850, 0.0052982,
        -0.9000405, 1.4253036, -0.0146949,
        -0.4706338, 0.0885814, 1.0093968);

    // 辐射去极化(自然光)
    static float Depol(const glm::vec2 &polV)
    {
        return 0.5f * (polV.x + polV.y);
    }

    // 辐射去极化(彩色光)
    static glm::vec3 DepolColor(const glm::vec3 &colS, const glm::vec3 &colP)
    {
        return 0.5f * (colS + colP);
    }

    void IridescentMaterial::FresnelDielectric(float cos_theta1, float n1, float n2, glm::vec2 &R, glm::vec2 &phi) const
    {
        float sin2_theta1 = 1.f - cos_theta1 * cos_theta1;
        float nr = n1 / n2;

        if (nr * nr * sin2_theta1 > 1.f) // Total internal reflection 全内反射
        {
            R = glm::vec2(1.f, 1.f);
            phi.x = 2.f * std::atan(-nr * nr * std::sqrt(sin2_theta1 - 1.f / (nr * nr)) / cos_theta1);
            phi.y = 2.f * std::atan(-std::sqrt(sin2_theta1 - 1.f / (nr * nr)) / cos_theta1);
        }
        else // Transmission & Reflection 透射与反射
        {
            float cos_theta2 = std::sqrt(1.f - nr * nr * sin2_theta1);
            glm::vec2 r;
            r.x = (n2 * cos_theta1 - n1 * cos_theta2) / (n2 * cos_theta1 + n1 * cos_theta2);
            r.y = (n1 * cos_theta1 - n2 * cos_theta2) / (n1 * cos_theta1 + n2 * cos_theta2);

            phi.x = (r.x < 0.f) ? PI : 0.f;
            phi.y = (r.y < 0.f) ? PI : 0.f;
            R = r * r;
        }
    }

    void IridescentMaterial::FresnelConductor(float cos_theta1, float n1, float n2, float k, glm::vec2 &R, glm::vec2 &phi) const
    {
        if (k == 0.f) // 当消光系数为零默认电介质
        {
            FresnelDielectric(cos_theta1, n1, n2, R, phi);
            return;
        }

        float A = n2 * n2 * (1.f - k * k) - n1 * n1 * (1.f - cos_theta1 * cos_theta1);
        float B = std::sqrt(A * A + 4.f * n2 * n2 * n2 * n2 * k * k);
        float U = std::sqrt((A + B) / 2.f);
        float V = std::sqrt((B - A) / 2.f);

        R.y = (n1 * cos_theta1 - U) * (n1 * cos_theta1 - U) + V * V;
        R.y /= (n1 * cos_theta1 + U) * (n1 * cos_theta1 + U) + V * V;
        phi.y = std::atan2(2.f * n1 * V * cos_theta1, U * U + V * V - n1 * n1 * cos_theta1 * cos_theta1) + PI;

        float n2_2 = n2 * n2;
        float n2_2_1mk2 = n2_2 * (1.f - k * k);
        float n2_2_2k = 2.f * n2_2 * k;

        float num_x = (n2_2_1mk2 * cos_theta1 - n1 * U) * (n2_2_1mk2 * cos_theta1 - n1 * U) + (n2_2_2k * cos_theta1 - n1 * V) * (n2_2_2k * cos_theta1 - n1 * V);
        float den_x = (n2_2_1mk2 * cos_theta1 + n1 * U) * (n2_2_1mk2 * cos_theta1 + n1 * U) + (n2_2_2k * cos_theta1 + n1 * V) * (n2_2_2k * cos_theta1 + n1 * V);
        R.x = num_x / den_x;

        float n2_2_1pk2_ct = n2_2 * (1.f + k * k) * cos_theta1;
        phi.x = std::atan2(2.f * n1 * n2_2 * cos_theta1 * (2.f * k * U - (1.f - k * k) * V), n2_2_1pk2_ct * n2_2_1pk2_ct - n1 * n1 * (U * U + V * V));
    }

    glm::vec3 IridescentMaterial::EvalSensitivity(float opd, float shift) const
    {
        // Use Gaussian fits for XYZ sensitivity curves in Fourier space
        float phase = 2.f * PI * opd * 1.0e-6f;

        glm::vec3 val(5.4856e-13f, 4.4201e-13f, 5.2481e-13f);
        glm::vec3 pos(1.6810e+06f, 1.7953e+06f, 2.2084e+06f);
        glm::vec3 var(4.3278e+09f, 9.3046e+09f, 6.6121e+09f);

        glm::vec3 xyz = val * std::sqrt(2.f * PI) * glm::sqrt(var);
        xyz *= glm::cos(pos * phase + shift);
        xyz *= glm::exp(-var * phase * phase);

        // Additional component for X
        xyz.x += 9.7470e-14f * std::sqrt(2.f * PI * 4.5282e+09f) * std::cos(2.2399e+06f * phase + shift) * std::exp(-4.5282e+09f * phase * phase);

        return xyz / 1.0685e-7f;
    }

    glm::vec3 IridescentMaterial::ComputeIridescence(float cos_theta1, float cos_theta2) const
    {
        // Force eta_2 -> 1.0 when Dinc -> 0.0
        float eta_2 = glm::mix(1.f, mEta2, glm::smoothstep(0.f, 0.03f, mDinc));

        // First interface (air to thin film)
        glm::vec2 R12, phi12;
        FresnelDielectric(cos_theta1, 1.f, eta_2, R12, phi12);
        glm::vec2 R21 = R12;
        glm::vec2 T121 = glm::vec2(1.f) - R12;
        glm::vec2 phi21 = glm::vec2(PI) - phi12;

        // Second interface (thin film to base material)
        glm::vec2 R23, phi23;
        FresnelConductor(cos_theta2, eta_2, mEta3, mKappa3, R23, phi23);

        // Phase shift due to optical path difference
        float OPD = mDinc * cos_theta2;
        glm::vec2 phi2 = phi21 + phi23;

        // Compound terms
        glm::vec3 I(0.f);
        glm::vec2 R123 = R12 * R23;
        glm::vec2 r123 = glm::sqrt(R123);
        glm::vec2 Rs = T121 * T121 * R23 / (glm::vec2(1.f) - R123);

        // Reflectance term for m=0 (DC term amplitude)
        glm::vec2 C0 = R12 + Rs;
        glm::vec3 S0 = EvalSensitivity(0.f, 0.f);
        I += Depol(C0) * S0;

        // Reflectance term for m>0 (pairs of diracs)
        glm::vec2 Cm = Rs - T121;
        for (int m = 1; m <= 3; ++m)
        {
            Cm *= r123;
            glm::vec3 SmS = 2.f * EvalSensitivity(m * OPD, m * phi2.x);
            glm::vec3 SmP = 2.f * EvalSensitivity(m * OPD, m * phi2.y);
            I += DepolColor(Cm.x * SmS, Cm.y * SmP);
        }

        // Convert back to RGB reflectance
        I = glm::clamp(XYZ_TO_RGB * I, glm::vec3(0.f), glm::vec3(1.f));
        return (mKappa3 < 0.01f) ? I * mBaseColor : I;
    }

    std::optional<BSDFInfo> IridescentMaterial::SampleBSDF(const glm::vec3 &hit_point, const glm::vec3 &view_dir, const RNG &rng) const
    {
        glm::vec3 microfacet_normal(0.f, 1.f, 0.f);
        if (!mMicrofacet.IsDeltaDistribution())
        {
            microfacet_normal = mMicrofacet.SampleVisibleNormal(view_dir, rng);
        }

        // Compute reflection direction
        glm::vec3 light_dir = -view_dir + 2.f * glm::dot(view_dir, microfacet_normal) * microfacet_normal;

        // Compute angles for thin-film interference
        float cos_theta1 = glm::abs(glm::dot(view_dir, microfacet_normal));
        float eta_2 = glm::mix(1.f, mEta2, glm::smoothstep(0.f, 0.03f, mDinc));
        float sin2_theta1 = 1.f - cos_theta1 * cos_theta1;
        float cos_theta2 = std::sqrt(glm::max(0.f, 1.f - (1.f / (eta_2 * eta_2)) * sin2_theta1));

        // Compute iridescent color
        glm::vec3 iridescent_color = ComputeIridescence(cos_theta1, cos_theta2);

        if (mMicrofacet.IsDeltaDistribution())
        {
            return BSDFInfo{iridescent_color / glm::abs(light_dir.y), 1.f, light_dir};
        }

        // Microfacet BRDF formula
        float D = mMicrofacet.D(microfacet_normal);
        float G = mMicrofacet.G2(light_dir, view_dir, microfacet_normal);
        glm::vec3 bsdf = iridescent_color * D * G / glm::abs(4.f * light_dir.y * view_dir.y);
        float pdf = mMicrofacet.VisibleNormalDistribution(view_dir, microfacet_normal) / glm::abs(4.0f * glm::dot(view_dir, microfacet_normal));

        return BSDFInfo{bsdf, pdf, light_dir};
    }

    glm::vec3 IridescentMaterial::BSDF(const glm::vec3 &hit_point, const glm::vec3 &light_dir, const glm::vec3 &view_dir) const
    {
        if (mMicrofacet.IsDeltaDistribution())
        {
            return {};
        }

        float lv = light_dir.y * view_dir.y;
        if (lv <= 0.f)
        {
            return {};
        }

        glm::vec3 microfacet_normal = glm::normalize(light_dir + view_dir);
        if (microfacet_normal.y <= 0.f)
        {
            microfacet_normal = -microfacet_normal;
        }

        // Compute angles for thin-film interference
        float cos_theta1 = glm::abs(glm::dot(view_dir, microfacet_normal));
        float eta_2 = glm::mix(1.f, mEta2, glm::smoothstep(0.f, 0.03f, mDinc));
        float sin2_theta1 = 1.f - cos_theta1 * cos_theta1;
        float cos_theta2 = std::sqrt(glm::max(0.f, 1.f - (1.f / (eta_2 * eta_2)) * sin2_theta1));

        // Compute iridescent color
        glm::vec3 iridescent_color = ComputeIridescence(cos_theta1, cos_theta2);

        // Microfacet BRDF formula
        float D = mMicrofacet.D(microfacet_normal);
        float G = mMicrofacet.G2(light_dir, view_dir, microfacet_normal);
        glm::vec3 bsdf = iridescent_color * D * G / glm::abs(4.f * lv);

        return bsdf;
    }

    float IridescentMaterial::PDF(const glm::vec3 &hit_point, const glm::vec3 &light_dir, const glm::vec3 &view_dir) const
    {
        if (mMicrofacet.IsDeltaDistribution())
        {
            return 0.f;
        }

        float lv = light_dir.y * view_dir.y;
        if (lv <= 0.f)
        {
            return 0.f;
        }

        glm::vec3 microfacet_normal = glm::normalize(light_dir + view_dir);
        if (microfacet_normal.y <= 0.f)
        {
            microfacet_normal = -microfacet_normal;
        }

        return mMicrofacet.VisibleNormalDistribution(view_dir, microfacet_normal) / glm::abs(4.f * glm::dot(view_dir, microfacet_normal));
    }

    void IridescentMaterial::Regularize() const
    {
        float alpha_x = mMicrofacet.GetAlphaX();
        float alpha_z = mMicrofacet.GetAlphaZ();

        if (alpha_x < 0.3f)
            alpha_x = glm::clamp(2.f * alpha_x, 0.1f, 0.3f);
        mMicrofacet.SetAlphaX(alpha_x);

        if (alpha_z < 0.3f)
            alpha_z = glm::clamp(2.f * alpha_z, 0.1f, 0.3f);
        mMicrofacet.SetAlphaZ(alpha_z);
    }
}