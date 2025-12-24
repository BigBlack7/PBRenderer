#pragma once

#include <vector>

namespace pbrt
{
    class AliasTable
    {
    private:
        struct Item
        {
            double __q__;
            union
            {
                float __p__;
                size_t __alias__;
            };
        };

        struct SampleResult
        {
            size_t __idx__;
            float __prob__;
        };

    private:
        std::vector<float> mProbs;
        std::vector<Item> mItems;

    public:
        AliasTable() = default;

        void Build(const std::vector<float> &values);
        SampleResult Sample(float u) const;

        const std::vector<float> &GetProbs() const { return mProbs; }
    };
}