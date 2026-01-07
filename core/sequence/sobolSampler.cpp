#include "sobolSampler.hpp"
#include "sobolmatrices.hpp"
#include "utils/logger.hpp"
#include <algorithm>
#include <limits>

namespace pbrt
{

    namespace
    {
        // Clamp to keep Sobol interval indices within the 52-bit direction table.
        constexpr uint32_t MaxSobolResolutionLog2 = 16;
        constexpr uint32_t HashMixConstant = 0x9e3779b9u;

        inline uint32_t ReverseBits32(uint32_t v)
        {
            v = (v << 16) | (v >> 16);
            v = ((v & 0x00ff00ffu) << 8) | ((v & 0xff00ff00u) >> 8);
            v = ((v & 0x0f0f0f0fu) << 4) | ((v & 0xf0f0f0f0u) >> 4);
            v = ((v & 0x33333333u) << 2) | ((v & 0xccccccccu) >> 2);
            v = ((v & 0x55555555u) << 1) | ((v & 0xaaaaaaaau) >> 1);
            return v;
        }

        struct FastOwenScrambler
        {
            explicit FastOwenScrambler(uint32_t seed) : __seed__(seed) {}
            uint32_t operator()(uint32_t v) const
            {
                // Constants follow PBRT-v4's fast Owen scrambling formulation.
                v = ReverseBits32(v);
                v ^= v * 0x3d20adea;
                v += __seed__;
                v *= (__seed__ >> 16) | 1;
                v ^= v * 0x05526c56;
                v ^= v * 0x53a22864;
                return ReverseBits32(v);
            }
            uint32_t __seed__;
        };

        template <typename Scrambler>
        inline float SobolSample(uint64_t index, int dimension, Scrambler scrambler)
        {
            int dim = std::min(dimension, NSobolDimensions - 1);
            uint32_t v = 0;
            for (int i = dim * SobolMatrixSize; index != 0 && i < (dim + 1) * SobolMatrixSize; ++i, index >>= 1)
            {
                if (index & 1)
                {
                    v ^= SobolMatrices32[i];
                }
            }
            v = scrambler(v);
            return std::min(v * 0x1p-32f, 0.99999994f);
        }

        inline uint64_t SobolIntervalToIndex(uint32_t log2_scale, uint64_t frame, const glm::ivec2 &p)
        {
            if (log2_scale == 0)
            {
                return frame;
            }

            const uint32_t m2 = log2_scale << 1;
            uint64_t index = frame << m2;

            uint64_t delta = 0;
            for (int c = 0; frame; frame >>= 1, ++c)
            {
                if (frame & 1)
                {
                    delta ^= VdCSobolMatrices[log2_scale - 1][c];
                }
            }

            uint64_t b = (static_cast<uint64_t>(static_cast<uint32_t>(p.x)) << log2_scale) | static_cast<uint32_t>(p.y);
            b ^= delta;

            for (int c = 0; b; b >>= 1, ++c)
            {
                if (b & 1)
                {
                    index ^= VdCSobolMatricesInv[log2_scale - 1][c];
                }
            }

            return index;
        }
    } // namespace

    glm::ivec2 SobolSampler::Resolution{0, 0};
    uint32_t SobolSampler::Log2Resolution = 0;

    SobolSampler::SobolSampler() : mSampleIndex(0), mDimension(0), mPixel(0, 0), mSeed(0) {}

    SobolSampler::SobolSampler(uint32_t seed) : mSampleIndex(0), mDimension(0), mPixel(0, 0), mSeed(seed) {}

        uint32_t SobolSampler::Hash(uint32_t a, uint32_t b)
    {
        uint32_t v = a * 374761393U + b * 668265263U;
        v ^= (v >> 15);
        v *= 0x45d9f3bU;
        v ^= (v >> 16);
        return v;
    }

    void SobolSampler::SetSampleExtent(const glm::ivec2 &resolution)
    {
        Resolution = resolution;
        uint32_t max_dim = static_cast<uint32_t>(std::max(resolution.x, resolution.y));
        uint32_t log2_scale = 0;
        while (log2_scale < MaxSobolResolutionLog2 && (1u << log2_scale) < max_dim)
        {
            ++log2_scale;
        }
        log2_scale = std::min(log2_scale, MaxSobolResolutionLog2);
        Log2Resolution = log2_scale;
    }

    float SobolSampler::SampleDimension(int dimension) const
    {
        int safe_dim = std::max(0, dimension);
        uint32_t scramble_seed = ComputeScrambleSeed(safe_dim);
        return SobolSample(mSampleIndex, safe_dim, FastOwenScrambler(scramble_seed));
    }

    uint32_t SobolSampler::ComputeScrambleSeed(int dimension) const
    {
        int safe_dim = std::max(0, dimension);
        uint32_t dim_index = static_cast<uint32_t>(std::min(safe_dim, NSobolDimensions - 1));
        uint64_t mix = static_cast<uint64_t>(dim_index) * HashMixConstant;
        uint32_t base_seed = Hash(static_cast<uint32_t>(mPixel.x), static_cast<uint32_t>(mPixel.y));
        return Hash(base_seed, mSeed + static_cast<uint32_t>(mix));
    }

    void SobolSampler::StartPixelSample(const glm::ivec2 &pixel, int sample_index)
    {
        mPixel = pixel;
        mDimension = 0;

        mLog2Resolution = Log2Resolution;
        if (mLog2Resolution == 0)
        {
            // Clamp negative coordinates to zero and convert from index to count.
            uint32_t max_coord = static_cast<uint32_t>(std::max(0, std::max(pixel.x, pixel.y)));
            if (max_coord < std::numeric_limits<uint32_t>::max())
            {
                ++max_coord;
            }
            while (mLog2Resolution < MaxSobolResolutionLog2 && (1u << mLog2Resolution) < max_coord)
            {
                ++mLog2Resolution;
            }
            mLog2Resolution = std::min(mLog2Resolution, MaxSobolResolutionLog2);
        }

        mSampleIndex = SobolIntervalToIndex(mLog2Resolution, static_cast<uint64_t>(sample_index), pixel);
    }

    float SobolSampler::Get1D() const
    {
        float result = SampleDimension(mDimension);
        ++mDimension;
        return result;
    }

    glm::vec2 SobolSampler::Get2D() const
    {
        float x = SampleDimension(mDimension);
        float y = SampleDimension(mDimension + 1);
        mDimension += 2;
        return glm::vec2(x, y);
    }

    std::unique_ptr<Sampler> SobolSampler::Clone() const
    {
        return std::make_unique<SobolSampler>(mSeed);
    }

    int SobolSampler::GetSampleIndex() const
    {
        if (mSampleIndex > static_cast<uint64_t>(std::numeric_limits<int>::max()))
        {
            static bool warned = false;
            if (!warned)
            {
                warned = true;
                PBRT_WARN("Sobol sample index {} exceeds int range; clamping to INT_MAX", mSampleIndex);
            }
            return std::numeric_limits<int>::max();
        }
        return static_cast<int>(mSampleIndex);
    }
}