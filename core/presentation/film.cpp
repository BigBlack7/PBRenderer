#include "film.hpp"
#include "image.hpp"
#include "thread/threadPool.hpp"
#include "utils/rgb.hpp"
#include "utils/logger.hpp"
#include <fstream>

namespace pbrt
{
    Film::Film(size_t width, size_t height) : mWidth(width), mHeight(height)
    {
        mPixels.resize(mWidth * mHeight);
    }

    std::vector<uint8_t> Film::GenerateRGBABuffer()
    {
        std::vector<uint8_t> buffer(mWidth * mHeight * 4);

        for (size_t y = 0; y < mHeight; y++)
        {
            for (size_t x = 0; x < mWidth; x++)
            {
                auto pixel = GetPixel(x, y);
                if (pixel.__sampleCount__ == 0)
                {
                    continue;
                }
                RGB rgb(pixel.__color__ / static_cast<double>(pixel.__sampleCount__));
                auto index = (y * mWidth + x) * 4;
                buffer[index + 0] = rgb.mRed;
                buffer[index + 1] = rgb.mGreen;
                buffer[index + 2] = rgb.mBlue;
                buffer[index + 3] = 255;
            }
        }

        return buffer;
    }

    void Film::Save(const std::filesystem::path &filename) const
    {
        std::vector<glm::vec3> buffer(mWidth * mHeight);
        MasterThreadPool.ParallelFor(mWidth, mHeight, [&](size_t x, size_t y)
                                     {
                                         auto pixel = GetPixel(x, y);
                                         // 防止NaN check导致该像素内无采样点
                                         if (pixel.__sampleCount__ == 0)
                                         {
                                             return;
                                         }
                                         buffer[y * mWidth + x] = pixel.__color__ / static_cast<double>(pixel.__sampleCount__);
                                         // end
                                     },
                                     false);
        MasterThreadPool.Wait();

        Image image(std::move(buffer), mWidth, mHeight);
        image.Save(filename);

        // 文件保存成功后获取绝对路径
        if (std::filesystem::exists(filename))
        {
            PBRT_INFO("Film saved to: {}", std::filesystem::canonical(filename).string());
        }
        else
        {
            PBRT_ERROR("Film saved but file path verification failed: {}", filename.string());
        }
    }
}