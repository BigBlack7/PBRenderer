#pragma once
#include <glm/glm.hpp>
#include <filesystem>

namespace pbrt
{
    class Image
    {
    private:
        std::vector<glm::vec3> mPixels;
        size_t mWidth, mHeight;

    private:
        void SavePPM(const std::filesystem::path &filename) const;
        void SaveEXR(const std::filesystem::path &filename) const;
        void SaveHDR(const std::filesystem::path &filename) const;

        void LoadEXR(const std::filesystem::path &filename);
        void LoadHDR(const std::filesystem::path &filename);

    public:
        Image(const std::filesystem::path &filename);
        Image(const std::vector<glm::vec3> &pixels, size_t width, size_t height) : mPixels(pixels), mWidth(width), mHeight(height) {}
        Image(std::vector<glm::vec3> &&pixels, size_t width, size_t height) : mPixels(pixels), mWidth(width), mHeight(height) {}

        // 像素访问
        glm::vec3 GetPixel(size_t x, size_t y) const { return mPixels[glm::clamp<size_t>(y, 0, mHeight - 1) * mWidth + glm::clamp<size_t>(x, 0, mWidth - 1)]; }
        glm::vec3 GetPixel(const glm::vec2 &point) const { return GetPixel(static_cast<size_t>(point.x), static_cast<size_t>(point.y)); }
        // 设置像素值时进行边界检查，防止越界访问
        void SetPixel(size_t x, size_t y, const glm::vec3 &val) { mPixels[glm::clamp<size_t>(y, 0, mHeight - 1) * mWidth + glm::clamp<size_t>(x, 0, mWidth - 1)] = val; }

        // 获取图像分辨率或尺寸
        size_t GetWidth() const { return mWidth; }
        size_t GetHeight() const { return mHeight; }
        glm::ivec2 GetResolution() const { return {mWidth, mHeight}; }
        
        void Save(const std::filesystem::path &filename) const;
    };
}