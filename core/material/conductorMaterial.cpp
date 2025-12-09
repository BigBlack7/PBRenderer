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

    std::optional<BSDFSample> ConductorMaterial::SampleBSDF(const glm::vec3 &hit_point, const glm::vec3 &view_dir, const RNG &rng) const
    {
        glm::vec3 microfacet_normal{0.f, 1.f, 0.f};
        if (!mMicrofacet.IsDeltaDistribution())
        {
            microfacet_normal = mMicrofacet.SampleVisibleNormal(view_dir, rng);
        }
        glm::vec3 F = Fresnel(mIOR, mK, glm::abs(glm::dot(view_dir, microfacet_normal)));
        glm::vec3 light_dir = -view_dir + 2.f * glm::dot(view_dir, microfacet_normal) * microfacet_normal;
        if (mMicrofacet.IsDeltaDistribution())
        {
            return BSDFSample{F / glm::abs(light_dir.y), 1.f, light_dir};
        }

        glm::vec3 bsdf = F * mMicrofacet.D(microfacet_normal) * mMicrofacet.G2(light_dir, view_dir, microfacet_normal) / glm::abs(4.f * light_dir.y * view_dir.y);
        float pdf = mMicrofacet.VisibleNormalDistribution(view_dir, microfacet_normal) / glm::abs(4.f * glm::dot(view_dir, microfacet_normal));
        return BSDFSample{bsdf, pdf, light_dir};
    }
}