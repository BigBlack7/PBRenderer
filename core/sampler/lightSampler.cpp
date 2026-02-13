#include "lightSampler.hpp"

namespace pbrt
{
    void LightSampler::Build(float scene_radius)
    {
        // 计算每个光源的功率构建AliasTable
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
    }

    std::optional<LightSampleInfo> LightSampler::Sample(float u) const
    {
        if (mLights.empty())
        {
            return std::nullopt;
        }
        auto sample_result = mAliasTable.Sample(u);
        return LightSampleInfo{
            .__light__ = mLights[sample_result.__idx__],
            .__prob__ = sample_result.__prob__
            // end
        };
    }
}