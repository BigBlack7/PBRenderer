#pragma once
#include "utils/rng.hpp"
#include "sequence/sampler.hpp"
#include <glm/glm.hpp>

namespace pbrt
{
    constexpr float PI = 3.14159265358979323846f;
    const float INV_PI = 0.31830988618379067154f;

    inline glm::vec2 UniformSampleUnitDisk(const glm::vec2 &u)
    {
        float r = glm::sqrt(u.x);
        float theta = 2.f * PI * u.y;
        return glm::vec2(r * glm::cos(theta), r * glm::sin(theta));
    }

    inline glm::vec3 CosineSampleHemisphere(const glm::vec2 &u)
    {
        float r = glm::sqrt(u.x);
        float phi = 2.f * PI * u.y;
        return {r * glm::cos(phi), glm::sqrt(1.f - r * r), r * glm::sin(phi)};
    }

    inline float CosineSampleHemispherePDF(const glm::vec3 &direcition)
    {
        return direcition.y * INV_PI;
    }

    // 参数化的半球均匀采样（固定消耗2个维度）
    // 适用于低差异序列
    inline glm::vec3 UniformSampleHemisphere(const glm::vec2 &u)
    {
        float z = u.x;
        float r = glm::sqrt(glm::max(0.f, 1.f - z * z));
        float phi = 2.f * PI * u.y;
        return glm::vec3(r * glm::cos(phi), z, r * glm::sin(phi));
    }

    // 参数化的球面均匀采样（固定消耗2个维度）
    // 适用于低差异序列
    inline glm::vec3 UniformSampleSphere(const glm::vec2 &u)
    {
        float z = 1.f - 2.f * u.x;
        float r = glm::sqrt(glm::max(0.f, 1.f - z * z));
        float phi = 2.f * PI * u.y;
        return glm::vec3(r * glm::cos(phi), z, r * glm::sin(phi));
    }

    // 拒绝采样版本的半球均匀采样
    // 注意：此版本消耗不确定数量的随机数，不适用于低差异序列
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

    // 拒绝采样版本的球面均匀采样
    // 注意：此版本消耗不确定数量的随机数，不适用于低差异序列
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