#include "infiniteLight.hpp"
#include "sampler/spherical.hpp"

namespace pbrt
{
    float InfiniteLight::Phi(float scene_radius) const
    {
        return 4.f * PI * PI * scene_radius * scene_radius * glm::max(mLe.r, glm::max(mLe.g, mLe.b));
    }

    std::optional<LightInfo> InfiniteLight::SampleLight(const glm::vec3 &surface_point, float scene_radius, const RNG &rng) const
    {
        glm::vec3 light_direction = UniformSampleSphere(rng);
        return LightInfo{surface_point + 2.f * scene_radius * light_direction, light_direction, mLe, 1.f / (4.f * PI)};
    }
}