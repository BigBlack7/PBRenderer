#pragma once
#include "utils/rng.hpp"
#include "sequence/sampler.hpp"
#include <glm/glm.hpp>

namespace pbrt
{
    constexpr float PI = 3.14159265358979323846f;
    constexpr float INV_PI = 0.31830988618379067154f;

    inline glm::vec2 UniformSampleUnitDisk(const glm::vec2 &u)
    {
        float r = glm::sqrt(u.x);
        float theta = 2.f * PI * u.y;
        return glm::vec2(r * glm::cos(theta), r * glm::sin(theta));
    }

    inline glm::vec3 CosineSampleHemisphere(const glm::vec2 &u) // 余弦重要性采样(马利方法)
    {
        float r = glm::sqrt(u.x);
        float phi = 2.f * PI * u.y;
        /*  圆盘上的点 (r, φ) → 半球上的点 (sinθcosφ, cosθ, sinθsinφ)
            坐标关系：
                x = r * cos(φ) = sinθ * cosφ
                y = √(1 - r²) = cosθ
                z = r * sin(φ) = sinθ * sinφ
            推导y:
                x² + y² + z² = 1, y=√(1 - x² - z²) = √(1 - r²)
        */
        return glm::vec3(r * glm::cos(phi), glm::sqrt(1.f - r * r), r * glm::sin(phi));
    }

    // 余弦重要性采样得到的方向的PDF
    inline float CosineSampleHemispherePDF(const glm::vec3 &direcition)
    {
        return direcition.y * INV_PI;
    }

    // 均匀采样半球(接受拒绝采样)
    inline glm::vec3 UniformSampleHemisphere(const RNG &rng)
    {
        glm::vec3 res;
        do
        {
            res = {rng.Uniform(), rng.Uniform(), rng.Uniform()};
            res = res * 2.f - 1.f;
        } while (glm::length(res) > 1.f);
        if (res.y < 0.f)
        {
            res.y = -res.y;
        }
        return glm::normalize(res);
    }

    // 均匀采样球面
    inline glm::vec3 UniformSampleSphere(const RNG &rng)
    {
        glm::vec3 res;
        do
        {
            res = {rng.Uniform(), rng.Uniform(), rng.Uniform()};
            res = res * 2.f - 1.f;
        } while (glm::length(res) > 1.f);
        return glm::normalize(res);
    }
}