#pragma once
#include <glm/glm.hpp>

namespace pt
{
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
    };
}