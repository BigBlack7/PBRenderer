#pragma once
#include "light.hpp"
#include "presentation/image.hpp"
#include "sampler/aliasTable.hpp"

namespace pbrt
{
    class EnvLight : public Light
    {
    private:
        const Image *mImage;
        float mStartPhi;      // 环境贴图起始phi角度偏移
        float mPrecomputePhi; // 预计算光功率φ
        AliasTable mAliasTable, mAliasTableMISC; // 根据环境贴图网格功率进行采样
        bool mCompensated;
        glm::ivec2 mGridCount;                  // 环境贴图网格数量
        static constexpr size_t mGridSize = 50; // 环境贴图网格边长
    private:
        glm::vec2 ImagePointFromDirection(const glm::vec3 &direction) const;   // 将方向向量映射到环境贴图上的点
        glm::vec3 DirectionFromImagePoint(const glm::vec2 &image_point) const; // 将环境贴图上的点映射到方向向量
        glm::ivec2 GirdIdxFromImagePoint(const glm::vec2 &image_point) const;  // 将环境贴图上的点映射到网格索引

    public:
        EnvLight(const Image *image, float start_phi = 0);
        bool Impossible() const override { return mCompensated; }
        LightType GetLightType() const override { return LightType::Environment; }
        float Phi(float scene_radius) const override { return mPrecomputePhi * scene_radius * scene_radius; }
        std::optional<LightInfo> SampleLight(const glm::vec3 &surface_point, float scene_radius, const RNG &rng, bool MISC) const override;
        glm::vec3 GetRadiance(const glm::vec3 &surface_point, const glm::vec3 &light_point, const glm::vec3 &normal) const override;
        float PDF(const glm::vec3 &surface_point, const glm::vec3 &light_point, const glm::vec3 &normal, bool MISC) const override;
    };
}