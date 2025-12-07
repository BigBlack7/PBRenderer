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
        float etai_div_etat = mIOR;
        glm::vec3 normal = {0.f, 1.f, 0.f};

        float cos_theta_t = view_dir.y;
        if (cos_theta_t < 0)
        {
            etai_div_etat = 1.f / mIOR;
            normal = {0.f, -1.f, 0.f};
            cos_theta_t = -cos_theta_t;
        }

        float cos_theta_i;
        float F = Fresnel(etai_div_etat, cos_theta_t, cos_theta_i);

        if (rng.Uniform() <= F) // 反射
        {
            glm::vec3 light_dir{-view_dir.x, view_dir.y, -view_dir.z};
            return BSDFSample{mAlbedoR / glm::abs(light_dir.y), 1.f, light_dir};
        }
        else // 折射
        {
            glm::vec3 light_dir{(-view_dir / etai_div_etat + (cos_theta_t / etai_div_etat - cos_theta_i) * normal)};
            return BSDFSample{mAlbedoT / (etai_div_etat * etai_div_etat) / glm::abs(light_dir.y), 1.f, light_dir};
        }
    }
}