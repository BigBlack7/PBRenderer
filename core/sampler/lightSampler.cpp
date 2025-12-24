#include "lightSampler.hpp"

namespace pbrt
{
    void LightSampler::Build(float scene_radius)
    {
        std::vector<float> Phis;
        Phis.reserve(mLights.size());
        for (const auto *light : mLights)
        {
            Phis.push_back(light->Phi(scene_radius));
        }
        mAliasTable.Build(Phis);
        const auto &probs = mAliasTable.GetProbs();
        for (size_t i = 0; i < mLights.size(); ++i)
        {
            mLightProbs.insert(std::make_pair(mLights[i], probs[i]));
        }

        Phis.clear();
        for (const auto *light : mLightsMISC)
        {
            Phis.push_back(light->Phi(scene_radius));
        }
        mAliasTableMISC.Build(Phis);
        const auto &probsMISC = mAliasTableMISC.GetProbs();
        for (size_t i = 0; i < mLightsMISC.size(); ++i)
        {
            mLightProbsMISC.insert(std::make_pair(mLightsMISC[i], probsMISC[i]));
        }
    }

    std::optional<LightSampleInfo> LightSampler::SampleLight(float u, bool MISC) const
    {
        if (MISC)
        {
            if (mLightsMISC.empty())
            {
                return {};
            }
            auto sample_result = mAliasTableMISC.Sample(u);
            return LightSampleInfo{mLightsMISC[sample_result.__idx__], sample_result.__prob__};
        }
        

        if (mLights.empty())
        {
            return {};
        }
        auto sample_result = mAliasTable.Sample(u);
        return LightSampleInfo{mLights[sample_result.__idx__], sample_result.__prob__};
    }
}
