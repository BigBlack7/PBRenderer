#include "conductorMaterial.hpp"
#include "utils/complex.hpp"
#include "sample/spherical.hpp"

namespace pbrt
{
    glm::vec3 ConductorMaterial::Fresnel(const glm::vec3 &ior, const glm::vec3 &k, const glm::vec3 &view_dir) const
    {
        glm::vec3 F{};
        for (size_t i = 0; i < 3; i++)
        {
            Complex etat_div_etai{ior[i], k[i]};
            float cos_theta_i = glm::clamp(view_dir.y, 0.f, 1.f);
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
        glm::vec3 light_dir{-view_dir.x, view_dir.y, -view_dir.z};
        glm::vec3 bsdf = Fresnel(mIOR, mK, view_dir) / glm::abs(light_dir.y);
        return BSDFSample{bsdf, 1.f, light_dir};
    }
}