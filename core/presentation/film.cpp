#include "film.hpp"
#include "thread/threadPool.hpp"
#include "utils/logger.hpp"
#include "utils/rgb.hpp"
#include <fstream>

namespace pbrt
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

        std::vector<uint8_t> buffer(mWidth * mHeight * 3);
        threadPool.ParallelFor(mWidth, mHeight, [&](size_t x, size_t y)
                               {
            auto pixel = GetPixel(x, y);
            if (pixel.__sampleCount__ == 0)
            {
                return;
            }
            
            RGB rgb(pixel.__color__ / static_cast<float>(pixel.__sampleCount__));
            auto idx = (y * mWidth + x) * 3;
            buffer[idx + 0] = rgb.mRed;
            buffer[idx + 1] = rgb.mGreen;
            buffer[idx + 2] = rgb.mBlue; }, false);

        threadPool.Wait();
        file.write(reinterpret_cast<const char *>(buffer.data()), buffer.size());

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
