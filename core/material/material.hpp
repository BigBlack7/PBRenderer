#pragma once
#include "utils/rng.hpp"
#include <glm/glm.hpp>
#include <optional>
namespace pbrt
{
    struct BSDFSample
    {
    public:
        glm::vec3 __bsdf__;
        float __pdf__;
        glm::vec3 __lightDirection__;
    };

    class Material
    {
    public:
        glm::vec3 mEmission{0.f, 0.f, 0.f};

    public:
        virtual std::optional<BSDFSample> SampleBSDF(const glm::vec3 &hit_point, const glm::vec3 &view_dir, const RNG &rng) const = 0;
        void SetEmission(const glm::vec3 &emission) { mEmission = emission; }
    };
}