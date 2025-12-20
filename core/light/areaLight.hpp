#pragma once
#include "light.hpp"
#include "shape/shape.hpp"

namespace pbrt
{
    class AreaLight : public Light
    {
    private:
        const Shape &mShape;
        bool mIsTwoSides;

    public:
        AreaLight(const Shape &shape, const glm::vec3 &Le, bool is_two_sides) : Light(Le), mShape(shape), mIsTwoSides(is_two_sides) {}

        float Phi(float scene_radius) const override;
        std::optional<LightInfo> SampleLight(const glm::vec3 &surface_point, float scene_radius, const RNG &rng) const override;
        glm::vec3 GetRadiance(const glm::vec3 &surface_point, const glm::vec3 &light_point, const glm::vec3 &normal) const override;

        const Shape &GetShape() const { return mShape; }
    };
}