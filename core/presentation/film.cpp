#include "film.hpp"
#include "utils/logger.hpp"
#include "utils/rgb.hpp"
#include <fstream>

namespace pt
{
    Film::Film(size_t width, size_t height) : mWidth(width), mHeight(height)
    {
        mPixels.resize(mWidth * mHeight);
    }

    void Film::Save(const std::filesystem::path &filename)
    {
        std::ofstream file(filename, std::ios::binary);

        file << "P6\n"
             << mWidth << " " << mHeight << "\n255\n";

        for (size_t y = 0; y < mHeight; y++)
        {
            for (size_t x = 0; x < mWidth; x++)
            {
                auto pixel = GetPixel(x, y);
                RGB rgb(pixel.__color__ / static_cast<float>(pixel.__sampleCount__));
                file << static_cast<uint8_t>(rgb.mRed)
                     << static_cast<uint8_t>(rgb.mGreen)
                     << static_cast<uint8_t>(rgb.mBlue);
            }
        }
        file.close();

        // 文件保存成功后获取绝对路径
        if (std::filesystem::exists(filename))
        {
            PBRT_INFO("Film saved to: {}", std::filesystem::canonical(filename).string());
        }
        else
        {
            PBRT_WARN("Film saved but file path verification failed: {}", filename.string());
        }
    }
}
