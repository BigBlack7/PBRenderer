#pragma once

#include <vector>

namespace pbrt
{
    class AliasTable
    {
    private:
        struct Item
        {
            float __q__;
            union
            {
                float __p__;
                int __alias__;
            };
        };

        struct SampleResult
        {
            int __idx__;
            float __prob__;
        };

    private:
        std::vector<float> mProbs;
        std::vector<Item> mItems;

    public:
        AliasTable() = default;

        void Build(const std::vector<float> &values);
        SampleResult Sample(float u) const;
    };
}