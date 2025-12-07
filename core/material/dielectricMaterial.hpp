#pragma once
#include "material.hpp"

namespace pbrt
{
    class DielectricMaterial : public Material
    {
    private:
        glm::vec3 mAlbedoR{};
        glm::vec3 mAlbedoT{};
        float mIOR{};

    private:
        float Fresnel(float etai_div_etat, float cos_theta_t, float &cos_theta_i) const;

    public:
        DielectricMaterial(const glm::vec3 &albedo, float ior) : mAlbedoR(albedo), mAlbedoT(albedo), mIOR(ior) {}
        DielectricMaterial(const glm::vec3 &albedo_r, const glm::vec3 &albedo_t, float ior) : mAlbedoR(albedo_r), mAlbedoT(albedo_t), mIOR(ior) {}
        std::optional<BSDFSample> SampleBSDF(const glm::vec3 &hit_point, const glm::vec3 &view_dir, const RNG &rng) const override;
    };
}