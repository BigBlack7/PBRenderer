#pragma once
#include <random>

namespace pbrt
{
    class PCG32
    {
    private:
        uint64_t mState{}, mInc{};

    public:
        void SetState(uint64_t init_state, uint64_t int_seq)
        {
            mState = 0U;
            mInc = (int_seq << 1u) | 1u;
            (*this)();
            mState += init_state;
            (*this)();
        }

        uint32_t operator()()
        {
            uint64_t old_state = mState;
            mState = old_state * 6364136223846793005ULL + mInc;
            uint32_t xor_shifted = ((old_state >> 18u) ^ old_state) >> 27u;
            uint32_t rot = old_state >> 59u;
            return (xor_shifted >> rot) | (xor_shifted << ((-rot) & 31));
        }

        static constexpr uint32_t min() { return 0; }
        static constexpr uint32_t max() { return std::numeric_limits<uint32_t>::max(); }
    };

    class RNG
    {
    private:
        mutable PCG32 mGen;
        mutable std::uniform_real_distribution<float> mUniformDistribution{0.f, 1.f};

    public:
        RNG(uint64_t init_state, uint64_t int_seq) { SetState(init_state, int_seq); }
        RNG() : RNG(0, 0) {};
        void SetState(uint64_t init_state, uint64_t int_seq) { mGen.SetState(init_state, int_seq); }
        float Uniform() const { return mUniformDistribution(mGen); }
    };
}