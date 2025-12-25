#pragma once
#include <random>

namespace pbrt
{
    class RNG
    {
    private:
        mutable std::mt19937 mGen;
        mutable std::uniform_real_distribution<float> mUniformDistribution{0.f, 1.f};

    public:
        RNG(size_t seed) { SetSeed(seed); }
        RNG() : RNG(0) {};

        void SetSeed(size_t seed) { mGen.seed(seed); }
        float Uniform() const { return mUniformDistribution(mGen); }
    };
}