#include "groundMaterial.hpp"
#include "sampler/spherical.hpp"

namespace pbrt
{
    std::optional<BSDFInfo> GroundMaterial::SampleBSDF(const glm::vec3 &hit_point, const glm::vec3 &view_dir, const RNG &rng) const
    {
        if (view_dir.y == 0.f)
        {
            return {};
        }

        glm::vec3 light_dir = CosineSampleHemisphere({rng.Uniform(), rng.Uniform()});
        float pdf = CosineSampleHemispherePDF(light_dir);
        glm::vec3 bsdf = mAlbedo * INV_PI;
        if ((static_cast<int>(glm::floor(hit_point.x * 8 + 0.5f)) % 8 == 0) || (static_cast<int>(glm::floor(hit_point.z * 8 + 0.5f)) % 8 == 0))
        {
            bsdf *= 0.1f;
        }
        return BSDFInfo{bsdf, pdf, light_dir * glm::sign(view_dir.y)};
    }

    glm::vec3 GroundMaterial::BSDF(const glm::vec3 &hit_point, const glm::vec3 &light_dir, const glm::vec3 &view_dir) const
    {
        if (light_dir.y * view_dir.y <= 0)
        {
            return {};
        }

        glm::vec3 bsdf = mAlbedo * INV_PI;
        if ((static_cast<int>(glm::floor(hit_point.x * 8 + 0.5f)) % 8 == 0) || (static_cast<int>(glm::floor(hit_point.z * 8 + 0.5f)) % 8 == 0))
        {
            bsdf *= 0.1f;
        }
        return bsdf;
    }
}