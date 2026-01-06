#include "specularMaterial.hpp"
#include "sampler/spherical.hpp"

namespace pbrt
{
    std::optional<BSDFInfo> SpecularMaterial::SampleBSDF(const glm::vec3 &hit_point, const glm::vec3 &view_dir, const RNG &rng) const
    {
        glm::vec3 light_dir{-view_dir.x, view_dir.y, -view_dir.z};
        glm::vec3 bsdf = mAlbedo / glm::abs(light_dir.y);
        return BSDFInfo{bsdf, 1.f, light_dir};
    }

    std::optional<BSDFInfo> SpecularMaterial::SampleBSDF(const glm::vec3 &hit_point, const glm::vec3 &view_dir, const Sampler &sequence) const
    {
        glm::vec3 light_dir{-view_dir.x, view_dir.y, -view_dir.z};
        glm::vec3 bsdf = mAlbedo / glm::abs(light_dir.y);
        return BSDFInfo{bsdf, 1.f, light_dir};
    }
}