#include "diffuseMaterial.hpp"
#include "sampler/spherical.hpp"

namespace pbrt
{
    std::optional<BSDFSample> DiffuseMaterial::SampleBSDF(const glm::vec3 &hit_point, const glm::vec3 &view_dir, const RNG &rng) const
    {
        glm::vec3 light_dir = CosineSampleHemisphere({rng.Uniform(), rng.Uniform()});
        float pdf = CosineSampleHemispherePDF(light_dir);
        glm::vec3 bsdf = mAlbedo * INV_PI;
        return BSDFSample{bsdf, pdf, light_dir};
    }
}