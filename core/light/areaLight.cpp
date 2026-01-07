#include "areaLight.hpp"
#include "sampler/spherical.hpp"

namespace pbrt
{
    float AreaLight::Phi(float scene_radius) const
    {
        return (mIsTwoSides ? 2.f : 1.f) * PI * mShape.GetArea() * glm::max(mLe.r, glm::max(mLe.g, mLe.b));
    }

    std::optional<LightInfo> AreaLight::SampleLight(const glm::vec3 &surface_point, float scene_radius, const RNG &rng, bool MISC) const
    {
        auto shape_sample = mShape.SampleShape(rng);
        if (!shape_sample.has_value())
        {
            return {};
        }
        glm::vec3 light_direction_raw = shape_sample->__point__ - surface_point;
        glm::vec3 light_direction = glm::normalize(light_direction_raw);
        float cos_theta = glm::dot(shape_sample->__normal__, -light_direction);
        if (cos_theta == 0.f)
        {
            return {};
        }
        if ((!mIsTwoSides) && cos_theta < 0.f)
        {
            return {};
        }
        float det_J = glm::abs(cos_theta / glm::dot(light_direction_raw, light_direction_raw));
        return LightInfo{shape_sample->__point__, light_direction, mLe, shape_sample->__pdf__ / det_J};
    }

    // 参数化采样接口：使用2D样本在形状表面采样
    std::optional<LightInfo> AreaLight::SampleLight(const glm::vec3 &surface_point, float scene_radius, float u_select, const glm::vec2 &u_surface, bool MISC) const
    {
        auto shape_sample = mShape.SampleShape(u_surface);
        if (!shape_sample.has_value())
        {
            return {};
        }
        glm::vec3 light_direction_raw = shape_sample->__point__ - surface_point;
        glm::vec3 light_direction = glm::normalize(light_direction_raw);
        float cos_theta = glm::dot(shape_sample->__normal__, -light_direction);
        if (cos_theta == 0.f)
        {
            return {};
        }
        if ((!mIsTwoSides) && cos_theta < 0.f)
        {
            return {};
        }
        float det_J = glm::abs(cos_theta / glm::dot(light_direction_raw, light_direction_raw));
        return LightInfo{shape_sample->__point__, light_direction, mLe, shape_sample->__pdf__ / det_J};
    }

    glm::vec3 AreaLight::GetRadiance(const glm::vec3 &surface_point, const glm::vec3 &light_point, const glm::vec3 &normal) const
    {
        float cos_theta = glm::dot(surface_point - light_point, normal);
        if (cos_theta == 0.f)
        {
            return {};
        }
        if ((!mIsTwoSides) && cos_theta < 0.f)
        {
            return {};
        }
        return mLe;
    }

    float AreaLight::PDF(const glm::vec3 &surface_point, const glm::vec3 &light_point, const glm::vec3 &normal, bool MISC) const
    {
        float cos_theta = glm::dot(surface_point - light_point, normal);
        if (cos_theta == 0.f)
        {
            return 0.f;
        }
        if ((!mIsTwoSides) && cos_theta < 0.f)
        {
            return 0.f;
        }
        glm::vec3 light_direction_raw = light_point - surface_point;
        float det_J = glm::abs(cos_theta / glm::dot(light_direction_raw, light_direction_raw));
        return mShape.PDF(surface_point, normal) / det_J;
    }
}