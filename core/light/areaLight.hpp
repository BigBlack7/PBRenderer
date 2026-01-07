#pragma once
#include "light.hpp"
#include "shape/shape.hpp"

namespace pbrt
{
    class AreaLight : public Light
    {
    private:
        const Shape &mShape;
        glm::vec3 mLe;
        bool mIsTwoSides;

    public:
        AreaLight(const Shape &shape, const glm::vec3 &Le, bool is_two_sides) : mShape(shape), mLe(Le), mIsTwoSides(is_two_sides) {}

        LightType GetLightType() const override { return LightType::Area; }
        float Phi(float scene_radius) const override;
        std::optional<LightInfo> SampleLight(const glm::vec3 &surface_point, float scene_radius, const RNG &rng, bool MISC) const override;
        // 参数化采样接口：使用2D样本在形状表面采样
        std::optional<LightInfo> SampleLight(const glm::vec3 &surface_point, float scene_radius, float u_select, const glm::vec2 &u_surface, bool MISC) const override;
        glm::vec3 GetRadiance(const glm::vec3 &surface_point, const glm::vec3 &light_point, const glm::vec3 &normal) const override;
        float PDF(const glm::vec3 &surface_point, const glm::vec3 &light_point, const glm::vec3 &normal, bool MISC) const override;

        const Shape &GetShape() const { return mShape; }
    };
}