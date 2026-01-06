#pragma once
#include "sampler.hpp"
#include <cstdint>
#include <vector>

namespace pbrt
{
    // Sobol quasi-random sampler with Owen scrambling
    // 比伪随机采样器提供更好的分层效果
    class SobolSampler : public Sampler
    {
    private:
        int mSampleIndex;
        mutable int mDimension;
        glm::ivec2 mPixel;
        uint32_t mSeed;

        // 预计算的32维Sobol序列生成矩阵
        static const uint32_t SobolMatrices32[32][32];

        static uint32_t OwenScramble(uint32_t v, uint32_t seed);

        // 计算Sobol序列的第n个样本在第d维上的值
        float SampleDimension(int dimension) const;

        // 哈希函数, 用于生成种子
        static uint32_t Hash(uint32_t a, uint32_t b);

    public:
        SobolSampler();
        explicit SobolSampler(uint32_t seed);

        float Get1D() const override;
        glm::vec2 Get2D() const override;
        void StartPixelSample(const glm::ivec2 &pixel, int sample_index) override;
        std::unique_ptr<Sampler> Clone() const override;
        int GetSampleIndex() const override { return mSampleIndex; }
    };
}