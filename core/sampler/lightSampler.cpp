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
    }

    std::optional<LightSampleInfo> LightSampler::SampleLight(float u) const
    {
        if(mLights.empty())
        {
            return {};
        }
        auto sample_result = mAliasTable.Sample(u);
        return LightSampleInfo{mLights[sample_result.__idx__], sample_result.__prob__};
    }
}
