#include "diffuseMaterial.hpp"
#include "sampler/spherical.hpp"

namespace pbrt
{
    std::optional<BSDFInfo> DiffuseMaterial::SampleBSDF(const glm::vec3 &hit_point, const glm::vec3 &view_dir, const RNG &rng) const
    {
        if (view_dir.y == 0.f)
        {
            return {};
        }

        glm::vec3 light_dir = CosineSampleHemisphere({rng.Uniform(), rng.Uniform()});
        float pdf = CosineSampleHemispherePDF(light_dir);
        glm::vec3 bsdf = mAlbedo * INV_PI;
        return BSDFInfo{bsdf, pdf, light_dir * glm::sign(view_dir.y)};
    }

    glm::vec3 DiffuseMaterial::BSDF(const glm::vec3 &hit_point, const glm::vec3 &light_dir, const glm::vec3 &view_dir) const
    {
        if (light_dir.y * view_dir.y <= 0)
        {
            return {};
        }

        return mAlbedo * INV_PI;
    }

    float DiffuseMaterial::PDF(const glm::vec3 &hit_point, const glm::vec3 &light_dir, const glm::vec3 &view_dir) const
    {
        if (light_dir.y * view_dir.y <= 0)
        {
            return 0.f;
        }
        return CosineSampleHemispherePDF(light_dir);
    }
}