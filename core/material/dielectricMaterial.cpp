#include "dielectricMaterial.hpp"

namespace pbrt
{
    float DielectricMaterial::Fresnel(float etai_div_etat, float cos_theta_t, float &cos_theta_i) const
    {
        float sin2_theta_t = 1 - cos_theta_t * cos_theta_t;
        float sin2_theta_i = sin2_theta_t / (etai_div_etat * etai_div_etat);
        if (sin2_theta_i >= 1)
        {
            return 1.f;
        }
        cos_theta_i = glm::sqrt(1 - sin2_theta_i);

        float r_parl = (cos_theta_i - etai_div_etat * cos_theta_t) / (cos_theta_i + etai_div_etat * cos_theta_t);
        float r_perp = (etai_div_etat * cos_theta_i - cos_theta_t) / (etai_div_etat * cos_theta_i + cos_theta_t);
        return (r_parl * r_parl + r_perp * r_perp) * 0.5f;
    }

    std::optional<BSDFSample> DielectricMaterial::SampleBSDF(const glm::vec3 &hit_point, const glm::vec3 &view_dir, const RNG &rng) const
    {
        if (mIOR == 1)
        {
            return BSDFSample{mAlbedoT / glm::abs(view_dir.y), 1.f, -view_dir};
        }

        float etai_div_etat = mIOR;
        glm::vec3 microfacet_normal{0.f, 1.f, 0.f};
        if (!mMicrofacet.IsDeltaDistribution())
        {
            microfacet_normal = mMicrofacet.SampleVisibleNormal(view_dir, rng);
        }

        float cos_theta_t = view_dir.y;
        float inverse = 1.f;
        if (cos_theta_t < 0)
        {
            etai_div_etat = 1.f / mIOR;
            inverse = -1.f;
            cos_theta_t = -cos_theta_t;
        }

        float cos_theta_i;
        float F = Fresnel(etai_div_etat, cos_theta_t, cos_theta_i);

        if (rng.Uniform() <= F) // 反射
        {
            glm::vec3 light_dir = -view_dir + 2.f * glm::dot(view_dir, microfacet_normal) * microfacet_normal;
            if (mMicrofacet.IsDeltaDistribution())
            {
                return BSDFSample{mAlbedoR / glm::abs(light_dir.y), 1.f, light_dir};
            }
            glm::vec3 brdf = mAlbedoR * mMicrofacet.D(microfacet_normal) * mMicrofacet.G2(light_dir, view_dir, microfacet_normal) / glm::abs(4.f * light_dir.y * view_dir.y);
            float pdf = mMicrofacet.VisibleNormalDistribution(view_dir, microfacet_normal) / glm::abs(4.f * glm::dot(view_dir, microfacet_normal));
            return BSDFSample{brdf, pdf, light_dir};
        }
        else // 折射
        {
            glm::vec3 light_dir{(-view_dir / etai_div_etat) + (cos_theta_t / etai_div_etat - cos_theta_i) * microfacet_normal * inverse};

            float det_J = etai_div_etat * etai_div_etat * glm::abs(glm::dot(light_dir, microfacet_normal)) / glm::pow(glm::abs(glm::dot(view_dir, microfacet_normal)) - etai_div_etat * etai_div_etat * glm::abs(glm::dot(light_dir, microfacet_normal)), 2.f);

            glm::vec3 btdf = mAlbedoT * det_J * mMicrofacet.D(microfacet_normal) * mMicrofacet.G2(light_dir, view_dir, microfacet_normal) * glm::abs(glm::dot(view_dir, microfacet_normal) / (light_dir.y * view_dir.y));

            float pdf = mMicrofacet.VisibleNormalDistribution(view_dir, microfacet_normal) * det_J;

            return BSDFSample{btdf / (etai_div_etat * etai_div_etat), pdf, light_dir};
            // return BSDFSample{btdf, pdf, light_dir};
        }
    }
}