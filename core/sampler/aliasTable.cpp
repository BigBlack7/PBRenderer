#include "aliasTable.hpp"
#include <glm/glm.hpp>

namespace pbrt
{
    void AliasTable::Build(const std::vector<float> &values)
    {
        double sum = 0;
        for (float v : values)
        {
            sum += v;
        }
        mProbs.resize(values.size());
        mItems.resize(values.size());
        std::vector<size_t> less, greater;
        for (size_t i = 0; i < values.size(); i++)
        {
            mProbs[i] = values[i] / sum; // 归一化概率

            // 初始化别名表项
            mItems[i].__q__ = 1.0;
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

        // 构建别名表
        while ((!less.empty()) && (!greater.empty()))
        {
            auto &item_less = mItems[less.back()];
            auto &item_greater = mItems[greater.back()];
            size_t greater_idx = greater.back();
            less.pop_back();
            greater.pop_back();

            item_less.__q__ = item_less.__p__;
            item_less.__alias__ = greater_idx;
            item_greater.__p__ -= (1.0 - item_less.__q__);
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
        // 整数部分作为索引, 浮点部分用于重定位概率比较
        size_t idx = glm::floor(glm::clamp<size_t>(u * mItems.size(), 0, mItems.size() - 1));
        u = glm::clamp<float>(u * mItems.size() - idx, 0.f, 1.f);
        const auto &item = mItems[idx];
        // 如果采样概率为1或者u小于采样概率, 则直接返回当前索引和概率
        if ((item.__q__ == 1.0) || (u < item.__q__))
        {
            return SampleResult{idx, mProbs[idx]};
        }
        // 否则返回别名索引和概率
        return SampleResult{item.__alias__, mProbs[item.__alias__]};
    }
}