#pragma once
#include <glm/glm.hpp>
#include <memory>

namespace pbrt
{
    // Base class for all samplers (pseudo-random and quasi-random)
    class Sampler
    {
    public:
        virtual ~Sampler() = default;

        // Get a single random sample in [0, 1)
        virtual float Get1D() = 0;

        // Get a 2D random sample in [0, 1)^2
        virtual glm::vec2 Get2D() = 0;

        // Initialize the sampler for a new pixel and sample index
        // pixel: the pixel coordinates
        // sample_index: the sample number for this pixel (0, 1, 2, ...)
        virtual void StartPixelSample(const glm::ivec2 &pixel, int sample_index) = 0;

        // Clone the sampler for use in another thread
        virtual std::unique_ptr<Sampler> Clone() const = 0;

        // Get the current sample index
        virtual int GetSampleIndex() const = 0;
    };
}
