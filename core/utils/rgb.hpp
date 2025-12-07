#pragma once
#include <glm/glm.hpp>
#include <array>
namespace pbrt
{
    class RGB Lerp(const RGB &a, const RGB &b, float t);

    class RGB
    {
    public:
        int mRed, mGreen, mBlue;

    public:
        RGB(int red, int green, int blue) : mRed(red), mGreen(green), mBlue(blue) {}

        RGB(const glm::vec3 &color)
        {
            float inv_gamma = 1.f / 2.2f;
            mRed = glm::clamp<int>(glm::pow(color.r, inv_gamma) * 255.f, 0, 255);
            mGreen = glm::clamp<int>(glm::pow(color.g, inv_gamma) * 255.f, 0, 255);
            mBlue = glm::clamp<int>(glm::pow(color.b, inv_gamma) * 255.f, 0, 255);
        }

        operator glm::vec3() const
        {
            float inv_channel = 1.f / 255.f;
            return glm::vec3{
                glm::pow(mRed * inv_channel, 2.2f),
                glm::pow(mGreen * inv_channel, 2.2f),
                glm::pow(mBlue * inv_channel, 2.2f)};
        }

        // 热力图, 用来dedug, 观察bvh的构建情况
        inline static RGB GenerateHeatMap(float t)
        {
            std::array<RGB, 25> color_pallet{
                RGB{68, 1, 84},
                RGB{71, 17, 100},
                RGB{72, 31, 112},
                RGB{71, 45, 123},
                RGB{68, 58, 131},

                RGB{64, 70, 136},
                RGB{59, 82, 139},
                RGB{54, 93, 141},
                RGB{49, 104, 142},
                RGB{44, 114, 142},

                RGB{40, 124, 142},
                RGB{36, 134, 142},
                RGB{33, 144, 140},
                RGB{31, 154, 138},
                RGB{32, 164, 134},

                RGB{39, 173, 129},
                RGB{53, 183, 121},
                RGB{71, 193, 110},
                RGB{93, 200, 99},
                RGB{117, 208, 84},

                RGB{143, 215, 68},
                RGB{170, 220, 50},
                RGB{199, 224, 32},
                RGB{227, 228, 24},
                RGB{253, 231, 37}};

            if (t < 0.f || t > 1.f)
            {
                return RGB{255, 0, 0};
            }
            float idx_f = t * (color_pallet.size() - 1);
            size_t idx = (size_t)glm::floor(idx_f);
            return Lerp(color_pallet[idx], color_pallet[idx + 1], glm::fract(idx_f));
        }
    };

    // 线性插值, 用于颜色的平滑过渡, 插值热力图
    inline RGB Lerp(const RGB &v0, const RGB &v1, float t)
    {
        return RGB{
            glm::clamp<int>(v0.mRed + (v1.mRed - v0.mRed) * t, 0, 255),
            glm::clamp<int>(v0.mGreen + (v1.mGreen - v0.mGreen) * t, 0, 255),
            glm::clamp<int>(v0.mBlue + (v1.mBlue - v0.mBlue) * t, 0, 255)};
    }
}