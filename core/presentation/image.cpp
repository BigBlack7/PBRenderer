#include "image.hpp"
#include "thread/threadPool.hpp"
#include "utils/rgb.hpp"
#include <ImfHeader.h>
#include <ImfChannelList.h>
#include <ImfInputFile.h>
#include <ImfFrameBuffer.h>
#include <ImfOutputFile.h>
#include <fstream>

namespace pbrt
{

    Image::Image(const std::filesystem::path &filename)
    {
        Imf::InputFile file(filename.string().c_str());
        auto data_window = file.header().dataWindow();
        mWidth = data_window.max.x - data_window.min.x + 1;
        mHeight = data_window.max.y - data_window.min.y + 1;
        mPixels.resize(mWidth * mHeight);

        // TODO: Check EXR file format, channel types, etc.
        Imf::FrameBuffer framebuffer{};
        framebuffer.insert("R", {Imf::FLOAT, reinterpret_cast<char *>(mPixels.data()) + offsetof(glm::vec3, r), sizeof(glm::vec3), sizeof(glm::vec3) * mWidth});
        framebuffer.insert("G", {Imf::FLOAT, reinterpret_cast<char *>(mPixels.data()) + offsetof(glm::vec3, g), sizeof(glm::vec3), sizeof(glm::vec3) * mWidth});
        framebuffer.insert("B", {Imf::FLOAT, reinterpret_cast<char *>(mPixels.data()) + offsetof(glm::vec3, b), sizeof(glm::vec3), sizeof(glm::vec3) * mWidth});
        file.setFrameBuffer(framebuffer);
        file.readPixels(data_window.min.y, data_window.max.y);
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
    }

    void Image::SavePPM(const std::filesystem::path &filename) const
    {
        std::ofstream file(filename, std::ios::binary);

        file << "P6\n"
             << mWidth << " " << mHeight << "\n255\n";

        std::vector<uint8_t> buffer(mWidth * mHeight * 3);
        threadPool.ParallelFor(mWidth, mHeight, [&](size_t x, size_t y)
                               {
                                   auto idx = (y * mWidth + x) * 3;
                                   RGB rgb(GetPixel(x, y));
                                   buffer[idx + 0] = rgb.mRed;
                                   buffer[idx + 1] = rgb.mGreen;
                                   buffer[idx + 2] = rgb.mBlue;
                                   // end
                               },
                               false);

        threadPool.Wait();
        file.write(reinterpret_cast<const char *>(buffer.data()), buffer.size());
    }

    void Image::SaveEXR(const std::filesystem::path &filename) const
    {
        Imf::Header header(static_cast<int>(mWidth), static_cast<int>(mHeight));
        header.channels().insert("R", Imf::Channel(Imf::FLOAT));
        header.channels().insert("G", Imf::Channel(Imf::FLOAT));
        header.channels().insert("B", Imf::Channel(Imf::FLOAT));
        header.compression() = Imf::ZIP_COMPRESSION;
        header.dataWindow() = {{0, 0}, {static_cast<int>(mWidth - 1), static_cast<int>(mHeight - 1)}};
        Imf::OutputFile file(filename.string().c_str(), header);

        Imf::FrameBuffer framebuffer{};
        framebuffer.insert("R", {Imf::FLOAT, const_cast<char *>(reinterpret_cast<const char *>(mPixels.data())) + offsetof(glm::vec3, r), sizeof(glm::vec3), sizeof(glm::vec3) * mWidth});
        framebuffer.insert("G", {Imf::FLOAT, const_cast<char *>(reinterpret_cast<const char *>(mPixels.data())) + offsetof(glm::vec3, g), sizeof(glm::vec3), sizeof(glm::vec3) * mWidth});
        framebuffer.insert("B", {Imf::FLOAT, const_cast<char *>(reinterpret_cast<const char *>(mPixels.data())) + offsetof(glm::vec3, b), sizeof(glm::vec3), sizeof(glm::vec3) * mWidth});
        file.setFrameBuffer(framebuffer);
        file.writePixels(mHeight);
    }
}