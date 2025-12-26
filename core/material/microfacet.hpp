#pragma once
#include "utils/rng.hpp"
#include <glm/glm.hpp>

namespace pbrt
{
    class Microfacet // Smith Model, 伸缩不变性
    {
    private:
        mutable float mAlphaX{};
        mutable float mAlphaZ{};

    private:
        float SlopeDistribution(const glm::vec2 &slope) const; // 法线斜率分布, 拉伸前的形状分布
        float Lambda(const glm::vec3 &dir_up) const;

    public:
        Microfacet(float alpha_x, float alpha_z);
        float D(const glm::vec3 &microfacet_normal) const;                                                         // 法线-GGX Distribution
        float G1(const glm::vec3 &view_dir, const glm::vec3 &microfacet_normal) const;                             // 掩蔽函数 Mask
        float G2(const glm::vec3 &light_dir, const glm::vec3 &view_dir, const glm::vec3 &microfacet_normal) const; // 高度相关阴影掩蔽函数 ShadowMask
        float IsDeltaDistribution() const;

        float VisibleNormalDistribution(const glm::vec3 &view_dir, const glm::vec3 &microfacet_normal) const; // 法线分布可见性函数
        glm::vec3 SampleVisibleNormal(const glm::vec3 &view_dir, const RNG &rng) const;                       // 采样可见法线

        float GetAlphaX() const { return mAlphaX; };
        float GetAlphaZ() const { return mAlphaZ; };
        void SetAlphaX(float alpha_x) const { mAlphaX = alpha_x; };
        void SetAlphaZ(float alpha_z) const { mAlphaZ = alpha_z; };
    };
}