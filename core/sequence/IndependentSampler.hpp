#pragma once
#include "sampler.hpp"
#include "utils/rng.hpp"

namespace pbrt
{
    class IndependentSampler : public Sampler
    {
    private:
        mutable RNG mRNG;
        int mSampleIndex;

    public:
        IndependentSampler() : mSampleIndex(0) {}
        explicit IndependentSampler(size_t seed) : mRNG(seed), mSampleIndex(0) {}

        float Get1D() const override { return mRNG.Uniform(); }

        glm::vec2 Get2D() const override { return glm::vec2(mRNG.Uniform(), mRNG.Uniform()); }

        void StartPixelSample(const glm::ivec2 &pixel, int sample_index) override
        {
            mSampleIndex = sample_index;
            // 基于像素坐标和样本索引生成确定性种子
            size_t seed = static_cast<size_t>(pixel.x + pixel.y * 10000 + sample_index * 10000 * 10000);
            mRNG.SetSeed(seed);
        }

        std::unique_ptr<Sampler> Clone() const override
        {
            return std::make_unique<IndependentSampler>();
        }

        int GetSampleIndex() const override { return mSampleIndex; }

        // 直接访问底层RNG: 提供对内部随机数生成器的直接访问
        RNG &GetRNG() { return mRNG; }
        const RNG &GetRNG() const { return mRNG; }
    };
}