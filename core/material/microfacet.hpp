#pragma once
#include "utils/rng.hpp"
#include <glm/glm.hpp>

namespace pbrt
{
    class Microfacet // Smith Model
    {
    private:
        mutable float mAlphaX{};
        mutable float mAlphaZ{};

    private:
        float SlopeDistribution(const glm::vec2 &slope) const; // 法线斜率分布, 拉伸前的形状分布
        float Lambda(const glm::vec3 &dir_up) const;           // 满足伸缩不变性、各向异性

    public:
        Microfacet(float alpha_x, float alpha_z);
        // 法线分布函数GGX Distribution, 满足伸缩不变性、各向异性
        float D(const glm::vec3 &microfacet_normal) const;
        // 掩蔽函数G1(o,m)描述微面元上出射光的遮挡 - 阴影函数G1(i,m)描述入射光的遮挡
        float G1(const glm::vec3 &view_dir, const glm::vec3 &microfacet_normal) const;
        // 与高度相关的阴影-掩蔽函数
        float G2(const glm::vec3 &light_dir, const glm::vec3 &view_dir, const glm::vec3 &microfacet_normal) const;
        // 可见法线分布函数
        float VisibleNormalDistribution(const glm::vec3 &view_dir, const glm::vec3 &microfacet_normal) const; 
        // 采样可见法线
        glm::vec3 SampleVisibleNormal(const glm::vec3 &view_dir, const RNG &rng) const;                       
        // 微表面法线分布是否为delta分布, 即镜面反射或折射
        bool IsDeltaDistribution() const;

        float GetAlphaX() const { return mAlphaX; };
        float GetAlphaZ() const { return mAlphaZ; };
        void SetAlphaX(float alpha_x) const { mAlphaX = alpha_x; };
        void SetAlphaZ(float alpha_z) const { mAlphaZ = alpha_z; };
    };
}