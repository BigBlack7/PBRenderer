#pragma once
#include <glm/glm.hpp>
namespace pt
{
    class Material
    {
    public:
        glm::vec3 mAlbedo{1.f, 1.f, 1.f};
        bool mIsSpecular{false};
        glm::vec3 mEmission{0.f, 0.f, 0.f};
    };
}