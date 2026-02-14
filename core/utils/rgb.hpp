#pragma once
#include <glm/glm.hpp>
#include <array>

namespace pbrt
{
    class RGB Lerp(const RGB &a, const RGB &b, float t);

    /*
        物理世界(线性)  →  人眼感知(非线性)  →  显示器输出
        [0.0, 1.0]         γ=2.2幂函数          [0.0, 1.0]
        线性渐变           编码压缩             sRGB显示空间
    */

    enum class ToneMappingType
    {
        Gamma,
        Reinhard,
        ACES
    };

    class RGB
    {
    private:
        inline static ToneMappingType mToneMappingType{ToneMappingType::Gamma};
        inline static float mExposure{1.f};

    public:
        int mRed, mGreen, mBlue; // 8位颜色分量 [0, 255]

    private:
        inline static glm::vec3 ApplyToneMapping(glm::vec3 color)
        {
            color = glm::max(color, glm::vec3(0.f));
            color *= mExposure;

            if (mToneMappingType == ToneMappingType::Reinhard)
            {
                color = color / (color + glm::vec3(1.f));
            }
            else if (mToneMappingType == ToneMappingType::ACES)
            {
                constexpr float aces_a = 2.51f;
                constexpr float aces_b = 0.03f;
                constexpr float aces_c = 2.43f;
                constexpr float aces_d = 0.59f;
                constexpr float aces_e = 0.14f;
                color = (color * (aces_a * color + aces_b)) / (color * (aces_c * color + aces_d) + aces_e);
            }

            return glm::clamp(color, glm::vec3(0.f), glm::vec3(1.f));
        }

    public:
        RGB(int red, int green, int blue) : mRed(red), mGreen(green), mBlue(blue) {}

        RGB(const glm::vec3 &color) // 转换到sRGB空间
        {
            glm::vec3 mapped = ApplyToneMapping(color);
            float inv_gamma = 1.f / 2.2f;
            mRed = glm::clamp<int>(glm::pow(mapped.r, inv_gamma) * 255.f, 0, 255);
            mGreen = glm::clamp<int>(glm::pow(mapped.g, inv_gamma) * 255.f, 0, 255);
            mBlue = glm::clamp<int>(glm::pow(mapped.b, inv_gamma) * 255.f, 0, 255);
        }

        operator glm::vec3() const // 转换到线性空间
        {
            float inv_channel = 1.f / 255.f;
            return glm::vec3{
                glm::pow(mRed * inv_channel, 2.2f),
                glm::pow(mGreen * inv_channel, 2.2f),
                glm::pow(mBlue * inv_channel, 2.2f)
                // end
            };
        }

        // 初始化pbrt::RGB::SetToneMapping(pbrt::ToneMappingType::ACES, 1.f);
        static void SetToneMapping(ToneMappingType type, float exposure = 1.f)
        {
            mToneMappingType = type;
            mExposure = glm::max(exposure, 0.f);
        }

        static ToneMappingType GetToneMapping() { return mToneMappingType; }

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
                RGB{253, 231, 37}
                // end
            };

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
            glm::clamp<int>(v0.mBlue + (v1.mBlue - v0.mBlue) * t, 0, 255)
            // end
        };
    }
}