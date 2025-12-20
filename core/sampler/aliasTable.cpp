#include "aliasTable.hpp"
#include <glm/glm.hpp>

namespace pbrt
{
    void AliasTable::Build(const std::vector<float> &values)
    {
        float sum = 0;
        for (float v : values)
        {
            sum += v;
        }
        mProbs.resize(values.size());
        mItems.resize(values.size());
        std::vector<size_t> less, greater;
        for (size_t i = 0; i < values.size(); i++)
        {
            mProbs[i] = values[i] / sum;
            mItems[i].__q__ = 1.f;
            mItems[i].__p__ = mProbs[i] * values.size();
            if (mItems[i].__p__ < 1.f)
            {
                less.push_back(i);
            }
            else if (mItems[i].__p__ > 1.f)
            {
                greater.push_back(i);
            }
        }

        while ((!less.empty()) && (!greater.empty()))
        {
            auto &item_less = mItems[less.back()];
            auto &item_greater = mItems[greater.back()];
            size_t greater_idx = greater.back();
            less.pop_back();
            greater.pop_back();

            item_less.__q__ = item_less.__p__;
            item_less.__alias__ = greater_idx;
            item_greater.__p__ -= (1.f - item_less.__q__);
            if (item_greater.__p__ < 1.f)
            {
                less.push_back(greater_idx);
            }
            else if (item_greater.__p__ > 1.f)
            {
                greater.push_back(greater_idx);
            }
        }
    }
    AliasTable::SampleResult AliasTable::Sample(float u) const
    {
        int idx = glm::floor(glm::clamp<int>(u * mItems.size(), 0.f, mItems.size() - 1));
        u = glm::clamp<float>(u * mItems.size() - idx, 0.f, 1.f);
        const auto &item = mItems[idx];
        if (u <= item.__q__)
        {
            return SampleResult{idx, mProbs[idx]};
        }
        return {item.__alias__, mProbs[item.__alias__]};
    }
}