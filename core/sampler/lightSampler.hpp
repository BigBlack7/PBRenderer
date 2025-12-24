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

        std::vector<const Light *> mLightsMISC;
        std::map<const Light *, float> mLightProbsMISC;
        AliasTable mAliasTableMISC;

    public:
        LightSampler() = default;

        void AddLight(const Light *light)
        {
            mLights.push_back(light);
            if (light->GetLightType() != LightType::Infinite)
            {
                mLightsMISC.push_back(light);
            }
        }

        void Build(float scene_radius);
        std::optional<LightSampleInfo> SampleLight(float u, bool MISC) const;

        float GetProb(const Light *light, bool MISC) const // PMF
        {
            if (MISC)
            {
                if (light->GetLightType()==LightType::Infinite)
                {
                    return 0.f;
                }
                return mLightProbsMISC.at(light);
            }
            
            return mLightProbs.at(light);
        }
    };
}
