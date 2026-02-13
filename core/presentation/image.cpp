#include "image.hpp"
#include "thread/threadPool.hpp"
#include "utils/rgb.hpp"
#include "utils/logger.hpp"

// OpenEXR
#include <ImfHeader.h>
#include <ImfChannelList.h>
#include <ImfInputFile.h>
#include <ImfFrameBuffer.h>
#include <ImfOutputFile.h>
#include <fstream>
// stb(sfml存在IMPLEMENTATION)
#include <stb_image.h>
#include <stb_image_write.h>

namespace pbrt
{

    Image::Image(const std::filesystem::path &filename)
    {
        auto ext = filename.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        if (ext == ".exr")
        {
            LoadEXR(filename);
        }
        else if (ext == ".hdr")
        {
            LoadHDR(filename);
        }
        else
        {
            // Unsupported format
            PBRT_ERROR("Unsupported image format: {}", filename.string());
        }
    }

    void Image::Save(const std::filesystem::path &filename) const
    {
        if (filename.extension() == ".ppm")
        {
            SavePPM(filename);
        }
        else if (filename.extension() == ".exr")
        {
            SaveEXR(filename);
        }
        else if (filename.extension() == ".hdr")
        {
            SaveHDR(filename);
        }
        else
        {
            // Unsupported format
            PBRT_ERROR("Unsupported image format: {}", filename.string());
        }
    }

    void Image::SavePPM(const std::filesystem::path &filename) const
    {
        std::ofstream file(filename, std::ios::binary);

        file << "P6\n"
             << mWidth << " " << mHeight << "\n255\n";

        std::vector<uint8_t> buffer(mWidth * mHeight * 3);
        MasterThreadPool.ParallelFor(mWidth, mHeight, [&](size_t x, size_t y)
                                     {
                                         auto idx = (y * mWidth + x) * 3;
                                         RGB rgb(GetPixel(x, y));
                                         buffer[idx + 0] = rgb.mRed;
                                         buffer[idx + 1] = rgb.mGreen;
                                         buffer[idx + 2] = rgb.mBlue;
                                         // end
                                     },
                                     false);

        MasterThreadPool.Wait();
        file.write(reinterpret_cast<const char *>(buffer.data()), buffer.size());
    }

    void Image::SaveEXR(const std::filesystem::path &filename) const
    {
        Imf::Header header(static_cast<int>(mWidth), static_cast<int>(mHeight));
        // 设置通道信息
        header.channels().insert("R", Imf::Channel(Imf::FLOAT));
        header.channels().insert("G", Imf::Channel(Imf::FLOAT));
        header.channels().insert("B", Imf::Channel(Imf::FLOAT));
        header.compression() = Imf::ZIP_COMPRESSION; // 使用ZIP压缩, 使得文件更小
        header.dataWindow() = {{0, 0}, {static_cast<int>(mWidth - 1), static_cast<int>(mHeight - 1)}};
        Imf::OutputFile file(filename.string().c_str(), header);

        Imf::FrameBuffer framebuffer{};
        framebuffer.insert("R", {Imf::FLOAT, const_cast<char *>(reinterpret_cast<const char *>(mPixels.data())) + offsetof(glm::vec3, r), sizeof(glm::vec3), sizeof(glm::vec3) * mWidth});
        framebuffer.insert("G", {Imf::FLOAT, const_cast<char *>(reinterpret_cast<const char *>(mPixels.data())) + offsetof(glm::vec3, g), sizeof(glm::vec3), sizeof(glm::vec3) * mWidth});
        framebuffer.insert("B", {Imf::FLOAT, const_cast<char *>(reinterpret_cast<const char *>(mPixels.data())) + offsetof(glm::vec3, b), sizeof(glm::vec3), sizeof(glm::vec3) * mWidth});
        file.setFrameBuffer(framebuffer);
        file.writePixels(mHeight);
    }

    void Image::SaveHDR(const std::filesystem::path &filename) const
    {
        std::vector<float> buffer(mWidth * mHeight * 3);

        for (size_t y = 0; y < mHeight; ++y)
        {
            for (size_t x = 0; x < mWidth; ++x)
            {
                const glm::vec3 &c = GetPixel(x, y);
                size_t idx = (y * mWidth + x) * 3;

                buffer[idx + 0] = c.r;
                buffer[idx + 1] = c.g;
                buffer[idx + 2] = c.b;
            }
        }

        int ok = stbi_write_hdr(
            filename.string().c_str(),
            static_cast<int>(mWidth),
            static_cast<int>(mHeight),
            3,
            buffer.data());

        if (!ok)
        {
            PBRT_ERROR("Failed to save HDR image: {}", filename.string());
        }
    }

    void Image::LoadEXR(const std::filesystem::path &filename)
    {
        Imf::InputFile file(filename.string().c_str());
        auto data_window = file.header().dataWindow();
        mWidth = data_window.max.x - data_window.min.x + 1;
        mHeight = data_window.max.y - data_window.min.y + 1;
        mPixels.resize(mWidth * mHeight);

        /*
            (x ,y) -> base + x * xStride + y * yStride
            确认RGB通道每一个像素值的存储地址
        */
        Imf::FrameBuffer framebuffer{};
        framebuffer.insert("R", {Imf::FLOAT, reinterpret_cast<char *>(mPixels.data()) + offsetof(glm::vec3, r), sizeof(glm::vec3), sizeof(glm::vec3) * mWidth});
        framebuffer.insert("G", {Imf::FLOAT, reinterpret_cast<char *>(mPixels.data()) + offsetof(glm::vec3, g), sizeof(glm::vec3), sizeof(glm::vec3) * mWidth});
        framebuffer.insert("B", {Imf::FLOAT, reinterpret_cast<char *>(mPixels.data()) + offsetof(glm::vec3, b), sizeof(glm::vec3), sizeof(glm::vec3) * mWidth});
        file.setFrameBuffer(framebuffer);
        file.readPixels(data_window.min.y, data_window.max.y);
    }

    void Image::LoadHDR(const std::filesystem::path &filename)
    {
        int width = 0, height = 0, channels = 0;

        // 强制读取为 float RGB
        float *data = stbi_loadf(
            filename.string().c_str(),
            &width,
            &height,
            &channels,
            3);

        if (!data)
        {
            PBRT_ERROR("Failed to load HDR image: {}", filename.string());
            return;
        }

        mWidth = static_cast<size_t>(width);
        mHeight = static_cast<size_t>(height);
        mPixels.resize(mWidth * mHeight);

        // stb库为row-major, 左上角为原点(0, 0)
        for (size_t y = 0; y < mHeight; ++y)
        {
            for (size_t x = 0; x < mWidth; ++x)
            {
                size_t idx = (y * mWidth + x) * 3;
                mPixels[y * mWidth + x] = glm::vec3(
                    data[idx + 0],
                    data[idx + 1],
                    data[idx + 2]);
            }
        }
        stbi_image_free(data);
    }
}