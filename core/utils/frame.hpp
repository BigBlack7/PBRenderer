#pragma once

#include <glm/glm.hpp>

namespace pbrt
{
    // 局部坐标系用以反射空间计算
    class Frame
    {
    private:
        glm::vec3 mX, mY, mZ;

    public:
        Frame(const glm::vec3 &normal /*世界空间法线*/);

        glm::vec3 LocalFromWorld(const glm::vec3 &world_dir) const;
        glm::vec3 WorldFromLocal(const glm::vec3 &local_dir) const;
    };
}