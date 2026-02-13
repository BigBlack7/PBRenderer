#include "conductorMaterial.hpp"
#include "utils/complex.hpp"
#include "sampler/spherical.hpp"

namespace pbrt
{
    glm::vec3 ConductorMaterial::Fresnel(const glm::vec3 &ior, const glm::vec3 &k, float cos_theta_i) const
    {
        glm::vec3 F{};
        for (size_t i = 0; i < 3; i++)
        {
            Complex etat_div_etai{ior[i], k[i]};
            cos_theta_i = glm::clamp(cos_theta_i, 0.f, 1.f);
            float sin2_theta_i = 1.f - cos_theta_i * cos_theta_i;
            Complex sin2_theta_t = sin2_theta_i / (etat_div_etai * etat_div_etai);
            Complex cos_theta_t = sqrt(1.f - sin2_theta_t);

            Complex r_parl = (etat_div_etai * cos_theta_i - cos_theta_t) / (etat_div_etai * cos_theta_i + cos_theta_t);
            Complex r_perp = (cos_theta_i - etat_div_etai * cos_theta_t) / (cos_theta_i + etat_div_etai * cos_theta_t);
            F[i] = 0.5f * (norm(r_parl) + norm(r_perp));
        }
        return F;
    }

    std::optional<BSDFInfo> ConductorMaterial::SampleBSDF(const glm::vec3 &hit_point, const glm::vec3 &view_dir, const RNG &rng) const
    {
        glm::vec3 microfacet_normal{0.f, 1.f, 0.f};
        if (!mMicrofacet.IsDeltaDistribution()) // 非Delta分布微表面模型, 则需要采样微表面法线(即足够光滑时不考虑微面元模型)
        {
            microfacet_normal = mMicrofacet.SampleVisibleNormal(view_dir, rng);
        }
        glm::vec3 F = Fresnel(mIOR, mK, glm::abs(glm::dot(view_dir, microfacet_normal)));
        glm::vec3 light_dir = -view_dir + 2.f * glm::dot(view_dir, microfacet_normal) * microfacet_normal;

        // 镜面反射
        if (mMicrofacet.IsDeltaDistribution())
        {
            return BSDFInfo{
                .__bsdf__ = F / glm::abs(light_dir.y),
                .__pdf__ = 1.f,
                .__lightDirection__ = light_dir
                // end
            };
        }

        glm::vec3 bsdf = F * mMicrofacet.D(microfacet_normal) * mMicrofacet.G2(light_dir, view_dir, microfacet_normal) / glm::abs(4.f * light_dir.y * view_dir.y);
        float pdf = mMicrofacet.VisibleNormalDistribution(view_dir, microfacet_normal) / glm::abs(4.f * glm::dot(view_dir, microfacet_normal));
        return BSDFInfo{
            .__bsdf__ = bsdf,
            .__pdf__ = pdf,
            .__lightDirection__ = light_dir
            // end
        };
    }

    glm::vec3 ConductorMaterial::BSDF(const glm::vec3 &hit_point, const glm::vec3 &light_dir, const glm::vec3 &view_dir) const
    {
        if (mMicrofacet.IsDeltaDistribution())
        {
            return {};
        }
        // 观察方向与光线方向是否位于同一个半球
        float lv = light_dir.y * view_dir.y;
        if (lv <= 0.f)
        {
            return {};
        }

        glm::vec3 microfacet_normal = glm::normalize(light_dir + view_dir);
        if (microfacet_normal.y <= 0.f) // 局部坐标系内保持微表面法线位于上半球
        {
            microfacet_normal = -microfacet_normal;
        }

        glm::vec3 F = Fresnel(mIOR, mK, glm::abs(glm::dot(view_dir, microfacet_normal)));
        glm::vec3 bsdf = F * mMicrofacet.D(microfacet_normal) * mMicrofacet.G2(light_dir, view_dir, microfacet_normal) / glm::abs(4.f * lv);
        return bsdf;
    }

    float ConductorMaterial::PDF(const glm::vec3 &hit_point, const glm::vec3 &light_dir, const glm::vec3 &view_dir) const
    {
        if (mMicrofacet.IsDeltaDistribution())
        {
            return 0.f;
        }
        // 观察方向与光线方向是否位于同一个半球
        float lv = light_dir.y * view_dir.y;
        if (lv <= 0.f)
        {
            return 0.f;
        }

        glm::vec3 microfacet_normal = glm::normalize(light_dir + view_dir);
        if (microfacet_normal.y <= 0.f) // 局部坐标系内保持微表面法线位于上半球
        {
            microfacet_normal = -microfacet_normal;
        }
        return mMicrofacet.VisibleNormalDistribution(view_dir, microfacet_normal) / glm::abs(4.f * glm::dot(view_dir, microfacet_normal));
    }
    void ConductorMaterial::Regularize() const
    {
        float alpha_x = mMicrofacet.GetAlphaX();
        float alpha_z = mMicrofacet.GetAlphaZ();

        if (alpha_x < 0.3f)
            alpha_x = glm::clamp(2 * alpha_x, 0.1f, 0.3f);
        mMicrofacet.SetAlphaX(alpha_x);

        if (alpha_z < 0.3f)
            alpha_z = glm::clamp(2 * alpha_z, 0.1f, 0.3f);
        mMicrofacet.SetAlphaZ(alpha_z);
    }
}