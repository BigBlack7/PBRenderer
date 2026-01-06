#pragma once
#include "sampler.hpp"
#include <cstdint>
#include <vector>
#include <limits>

namespace pbrt
{
    // Sobol quasi-random sampler with Owen scrambling
    // 比伪随机采样器提供更好的分层效果
    class SobolSampler : public Sampler
    {
    private:
        uint64_t mSampleIndex;
        mutable int mDimension;
        glm::ivec2 mPixel;
        uint32_t mSeed;
        uint32_t mLog2Resolution;

        static glm::ivec2 sResolution;
        static uint32_t sLog2Resolution;

        float SampleDimension(int dimension) const;
        uint32_t ComputeScrambleSeed(int dimension) const;

        static uint32_t Hash(uint32_t a, uint32_t b);

    public:
        SobolSampler();
        explicit SobolSampler(uint32_t seed);

        static void SetSampleExtent(const glm::ivec2 &resolution);

        float Get1D() const override;
        glm::vec2 Get2D() const override;
        void StartPixelSample(const glm::ivec2 &pixel, int sample_index) override;
        std::unique_ptr<Sampler> Clone() const override;
        int GetSampleIndex() const override;
    };
}
