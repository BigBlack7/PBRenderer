#include "groundMaterial.hpp"
#include "sample/spherical.hpp"

namespace pbrt
{
    std::optional<BSDFSample> GroundMaterial::SampleBSDF(const glm::vec3 &hit_point, const glm::vec3 &view_dir, const RNG &rng) const
    {
        glm::vec3 light_dir = CosineSampleHemisphere({rng.Uniform(), rng.Uniform()});
        float pdf = CosineSampleHemispherePDF(light_dir);
        glm::vec3 bsdf = mAlbedo * INV_PI;
        if ((static_cast<int>(glm::floor(hit_point.x * 8)) % 8 == 0) || (static_cast<int>(glm::floor(hit_point.z * 8)) % 8 == 0))
        {
            bsdf *= 0.1f;
        }
        return BSDFSample{bsdf, pdf, light_dir};
    }
}