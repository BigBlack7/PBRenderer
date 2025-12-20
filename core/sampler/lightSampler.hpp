#pragma once
#include "light/light.hpp"
#include "aliasTable.hpp"
#include <optional>

namespace pbrt
{
    struct LightSampleInfo
    {
    public:
        const Light *__light__;
        float __prob__;
    };

    class LightSampler
    {
    private:
        std::vector<const Light *> mLights;
        AliasTable mAliasTable;

    public:
        LightSampler() = default;

        void AddLight(const Light *light)
        {
            mLights.push_back(light);
        }

        void Build(float scene_radius);
        std::optional<LightSampleInfo> SampleLight(float u) const;
    };
}
