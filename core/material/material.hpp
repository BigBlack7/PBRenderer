#pragma once
#include "utils/rng.hpp"
#include <glm/glm.hpp>
#include <optional>

namespace pbrt
{
    struct BSDFInfo
    {
    public:
        glm::vec3 __bsdf__;
        float __pdf__;
        glm::vec3 __lightDirection__;
        float __etaScale__{1.f};
    };

    class Material
    {
    public:
        const class AreaLight *mAreaLight{nullptr}; // 前向声明

    public:
        virtual std::optional<BSDFInfo> SampleBSDF(const glm::vec3 &hit_point, const glm::vec3 &view_dir, const RNG &rng) const = 0;
        virtual glm::vec3 BSDF(const glm::vec3 &hit_point, const glm::vec3 &light_dir, const glm::vec3 &view_dir) const = 0;
        virtual float PDF(const glm::vec3 &hit_point, const glm::vec3 &light_dir, const glm::vec3 &view_dir) const = 0;
        virtual bool IsDeltaDistribution() const = 0;
    };
}