#include "infiniteLight.hpp"
#include "sampler/spherical.hpp"

namespace pbrt
{
    float InfiniteLight::Phi(float scene_radius) const
    {
        return 4.f * PI * PI * scene_radius * scene_radius * glm::max(mLe.r, glm::max(mLe.g, mLe.b));
    }

    std::optional<LightInfo> InfiniteLight::SampleLight(const glm::vec3 &surface_point, float scene_radius, const RNG &rng, bool MISC) const
    {
        if (MISC)
        {
            return {};
        }
        
        glm::vec3 light_direction = UniformSampleSphere(rng);
        return LightInfo{surface_point + 2.f * scene_radius * light_direction, light_direction, mLe, 1.f / (4.f * PI)};
    }

    // 参数化采样接口：使用2D样本进行球面均匀采样
    std::optional<LightInfo> InfiniteLight::SampleLight(const glm::vec3 &surface_point, float scene_radius, float u_select, const glm::vec2 &u_surface, bool MISC) const
    {
        if (MISC)
        {
            return {};
        }

        glm::vec3 light_direction = UniformSampleSphere(u_surface);
        return LightInfo{surface_point + 2.f * scene_radius * light_direction, light_direction, mLe, 1.f / (4.f * PI)};
    }

    float InfiniteLight::PDF(const glm::vec3 &surface_point, const glm::vec3 &light_point, const glm::vec3 &normal, bool MISC) const
    {
        if (MISC)
        {
            return 0.f;
        }
        return 0.25f * INV_PI;
    }

    glm::vec3 InfiniteLight::GetRadiance(const glm::vec3 &surface_point, const glm::vec3 &light_point, const glm::vec3 &normal) const
    {
        return mLe;
    }
}