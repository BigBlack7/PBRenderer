#pragma once
#include "sampler.hpp"
#include <cstdint>
#include <vector>

namespace pbrt
{
    // Sobol quasi-random sampler with Owen scrambling
    // Provides better stratification than pseudo-random samplers
    class SobolSampler : public Sampler
    {
    private:
        int mSampleIndex;
        int mDimension;
        glm::ivec2 mPixel;
        uint32_t mSeed;

        // Sobol direction numbers (first 32 dimensions)
        // Pre-computed values for efficient generation
        static const uint32_t SobolMatrices32[32][32];

        // Owen scrambling for better distribution
        static uint32_t OwenScramble(uint32_t v, uint32_t seed);

        // Generate Sobol sample for given dimension
        float SampleDimension(int dimension) const;

        // Hash function for seed generation
        static uint32_t Hash(uint32_t a, uint32_t b);

    public:
        SobolSampler();
        explicit SobolSampler(uint32_t seed);

        float Get1D() override;
        glm::vec2 Get2D() override;
        void StartPixelSample(const glm::ivec2 &pixel, int sample_index) override;
        std::unique_ptr<Sampler> Clone() const override;
        int GetSampleIndex() const override { return mSampleIndex; }
    };
}
