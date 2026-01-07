#pragma once
#include "utils/rng.hpp"
#include "sequence/rngSampler.hpp"
#include <glm/glm.hpp>
#include <optional>
#include <cstring>

namespace pbrt
{
    struct LightInfo
    {
        glm::vec3 __lightPoint__;
        glm::vec3 __direction__;
        glm::vec3 __Le__;
        float __pdf__;
    };

    enum class LightType
    {
        Area,
        Infinite,
        Environment
    };

    class Light
    {
    public:
        virtual LightType GetLightType() const = 0;
        virtual float Phi(float scene_radius) const = 0; // 光源功率 radiant flux
        virtual std::optional<LightInfo> SampleLight(const glm::vec3 &surface_point, float scene_radius, const RNG &rng, bool MISC) const = 0;

        // 参数化采样接口：使用多个1D/2D样本作为输入
        // u_select: 用于离散选择（如环境光贴图网格选择）
        // u_surface: 用于光源表面采样
        // 默认实现：创建一个临时RNG并调用RNG版本
        // 注意：这不是最优的，子类应该覆盖此方法
        virtual std::optional<LightInfo> SampleLight(const glm::vec3 &surface_point, float scene_radius, float u_select, const glm::vec2 &u_surface, bool MISC) const
        {
            // 使用更健壮的哈希函数组合浮点数值生成种子
            auto hashFloat = [](float f) -> uint32_t {
                uint32_t bits;
                std::memcpy(&bits, &f, sizeof(bits));
                return bits;
            };
            uint32_t seed = hashFloat(u_select) ^ (hashFloat(u_surface.x) * 0x9e3779b9u) ^ (hashFloat(u_surface.y) * 0x85ebca6bu);
            thread_local RNG rng{};
            rng.SetSeed(seed);
            return SampleLight(surface_point, scene_radius, rng, MISC);
        }

        virtual float PDF(const glm::vec3 &surface_point, const glm::vec3 &light_point, const glm::vec3 &normal, bool MISC) const = 0;

        virtual glm::vec3 GetRadiance(const glm::vec3 &surface_point, const glm::vec3 &light_point, const glm::vec3 &normal) const = 0;
    };
}