#pragma once
#include <vector>

namespace pbrt
{
    class AliasTable
    {
    private:
        struct Item
        {
            double __q__; // 采样概率(重定位概率为1 - Q)
            union         // 同时期只访问一个成员
            {
                float __p__;      // 概率 * 样本数
                size_t __alias__; // 别名索引
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