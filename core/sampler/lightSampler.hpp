#pragma once
#include "light/light.hpp"
#include "aliasTable.hpp"
#include <optional>
#include <map>

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
        std::map<const Light *, float> mLightProbs;
        AliasTable mAliasTable;

    public:
        LightSampler() = default;

        void AddLight(const Light *light)
        {
            mLights.push_back(light);
        }

        void Build(float scene_radius);
        std::optional<LightSampleInfo> SampleLight(float u) const;

        float GetProb(const Light *light) const // PMF
        {
            auto res = mLightProbs.find(light);
            if (res == mLightProbs.end())
            {
                return 0.f;
            }
            return res->second;
        }
    };
}