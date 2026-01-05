#pragma once
#include "sampler.hpp"
#include "utils/rng.hpp"

namespace pbrt
{
    // Adapter class to use RNG as a Sampler
    // Provides backward compatibility with existing code
    class RNGSampler : public Sampler
    {
    private:
        mutable RNG mRNG;
        int mSampleIndex;

    public:
        RNGSampler() : mSampleIndex(0) {}
        explicit RNGSampler(size_t seed) : mRNG(seed), mSampleIndex(0) {}

        float Get1D() override { return mRNG.Uniform(); }
        
        glm::vec2 Get2D() override { return glm::vec2(mRNG.Uniform(), mRNG.Uniform()); }
        
        void StartPixelSample(const glm::ivec2 &pixel, int sample_index) override
        {
            mSampleIndex = sample_index;
            // Seed based on pixel coordinates and sample index
            size_t seed = static_cast<size_t>(pixel.x + pixel.y * 10000 + sample_index * 10000 * 10000);
            mRNG.SetSeed(seed);
        }
        
        std::unique_ptr<Sampler> Clone() const override
        {
            return std::make_unique<RNGSampler>();
        }
        
        int GetSampleIndex() const override { return mSampleIndex; }
        
        // Direct access to underlying RNG for compatibility
        RNG& GetRNG() { return mRNG; }
        const RNG& GetRNG() const { return mRNG; }
    };
}
