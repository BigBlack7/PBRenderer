#include "areaLight.hpp"
#include "sampler/spherical.hpp"
#include "shape/model.hpp"

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
            return std::nullopt;
        }
        glm::vec3 light_direction_raw = shape_sample->__point__ - surface_point;
        glm::vec3 light_direction = glm::normalize(light_direction_raw);
        float cos_theta = glm::dot(shape_sample->__normal__, -light_direction);
        // 光线方向掠射物体表面
        if (cos_theta == 0.f)
        {
            return std::nullopt;
        }
        // 单面光源且背面对着物体表面
        if ((!mIsTwoSides) && cos_theta < 0.f)
        {
            return std::nullopt;
        }
        float det_J = glm::abs(cos_theta / glm::dot(light_direction_raw, light_direction_raw));
        return LightInfo{
            .__lightPoint__ = shape_sample->__point__,
            .__direction__ = light_direction,
            .__Le__ = mLe,
            .__pdf__ = shape_sample->__pdf__ / det_J // 除以雅可比矩阵是将pdf从单位面积变换到单位立方体
            // end
        };
    }

    glm::vec3 AreaLight::GetRadiance(const glm::vec3 &surface_point, const glm::vec3 &light_point, const glm::vec3 &normal) const
    {
        float cos_theta = glm::dot(surface_point - light_point, normal);
        // 光线方向掠射物体表面
        if (cos_theta == 0.f)
        {
            return {};
        }
        // 单面光源且背面对着物体表面
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