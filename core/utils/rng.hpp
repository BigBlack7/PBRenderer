#pragma once
#include <random>
#include <pcg_random.hpp>

namespace pbrt
{
    // class RNG
    // {
    // private:
    //     mutable std::mt19937 mGen;
    //     mutable std::uniform_real_distribution<float> mUniformDistribution{0.f, 1.f};

    // public:
    //     RNG(size_t seed) { SetSeed(seed); }
    //     RNG() : RNG(0) {};
    //     void SetSeed(size_t seed) { mGen.seed(seed); }
    //     float Uniform() const { return mUniformDistribution(mGen); }
    // };

    class RNG
    {
    private:
        mutable pcg32 mGen;

    private:
        // splitmix64: 种子扩散/哈希, 用来把线性seed打散成高质量64bit
        static inline uint64_t SplitMix64(uint64_t &x)
        {
            uint64_t z = (x += 0x9e3779b97f4a7c15ull);
            z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ull;
            z = (z ^ (z >> 27)) * 0x94d049bb133111ebull;
            return z ^ (z >> 31);
        }

        // 将size_t seed扩散成 (state, stream_seed)
        static inline void MakePCGSeeds(size_t seed, uint64_t &state, uint64_t &stream_seed)
        {
            // 不使用std::hash, 在不同标准库/不同版本可能不稳定, 且质量也不保证
            uint64_t x = static_cast<uint64_t>(seed);

            // 用splitmix64连续产生两个64-bit值
            uint64_t s0 = SplitMix64(x);
            uint64_t s1 = SplitMix64(x);

            state = s0;

            // pcg32 是 setseq(specific_stream),第二个参数是"stream_seed"(不是inc本身)
            // 官方实现内部会执行 (stream_seed<<1)|1 来保证增量为奇数
            stream_seed = s1;
        }

    public:
        RNG(size_t seed) { SetSeed(seed); }
        RNG() : RNG(0) {}

        void SetSeed(size_t seed)
        {
            uint64_t state, stream_seed;
            MakePCGSeeds(seed, state, stream_seed);
            mGen.seed(state, stream_seed);

            // 丢弃前几个输出，增加种子扩散效果
            for (size_t i = 0; i < 4; ++i)
                (void)mGen();
        }

        // 生成严格的 [0,1) 浮点随机数
        float Uniform() const
        {
            // 取 32-bit 整数
            uint32_t x = mGen();

            // 映射到[0,1): 使用2^-32, 确保永远小于1
            // 使用double可减少float舍入造成的条纹/偏差风险
            constexpr double INV_2_POW_32 = 1. / 4294967296.; // 1 / 2^32
            double u = static_cast<double>(x) * INV_2_POW_32; // u∈[0,1)
            float uf = static_cast<float>(u);

            // 避免由于某些平台的奇怪fast-math导致uf==1
            // nextafterf(1,0)∈[0,1), 是1的前一个float
            uf = std::min(uf, std::nextafterf(1.f, 0.f));
            return uf;
        }
    };
}