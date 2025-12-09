#pragma once

#include <vector>
#include <filesystem>
#include <glm/glm.hpp>

namespace pbrt
{
    struct Pixel
    {
    public:
        glm::vec3 __color__{0.f, 0.f, 0.f};
        int __sampleCount__{0};

    public:
    };

    class Film
    {
    private:
        size_t mWidth;
        size_t mHeight;
        std::vector<Pixel> mPixels;

    public:
        Film(size_t width, size_t height);
        void Save(const std::filesystem::path &filename);

        size_t GetWidth() const { return mWidth; }
        size_t GetHeight() const { return mHeight; }

        Pixel GetPixel(size_t x, size_t y) const { return mPixels[y * mWidth + x]; }
        void AddSample(size_t x, size_t y, const glm::vec3 &color)
        {
            if (glm::any(glm::isnan(color)))
            {
                return;
            }
            mPixels[y * mWidth + x].__color__ += color;
            mPixels[y * mWidth + x].__sampleCount__++;
        }

        void Clear()
        {
            mPixels.clear();
            mPixels.resize(mWidth * mHeight);
        }
    };
}
