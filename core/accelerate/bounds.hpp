#pragma once
#include "light/ray.hpp"

namespace pbrt
{
    struct Bounds
    {
    public:
        glm::vec3 __bMin__; // 最小点 (x_min, y_min, z_min)
        glm::vec3 __bMax__; // 最大点 (x_max, y_max, z_max)

    public:
        Bounds() : __bMin__(std::numeric_limits<float>::max()), __bMax__(-std::numeric_limits<float>::max()) {}
        Bounds(const glm::vec3 &b_min, const glm::vec3 &b_max) : __bMin__(b_min), __bMax__(b_max) {}

        void Expand(const glm::vec3 &position)
        {
            // 扩展包围盒以包含点 position
            __bMin__ = glm::min(__bMin__, position);
            __bMax__ = glm::max(__bMax__, position);
        }

        void Expand(const Bounds &bounds)
        {
            // 扩展包围盒以包含另一个包围盒 bounds
            __bMin__ = glm::min(__bMin__, bounds.__bMin__);
            __bMax__ = glm::max(__bMax__, bounds.__bMax__);
        }

        bool HasIntersection(const Ray &ray, float t_min, float t_max) const;
        bool HasIntersection(const Ray &ray, const glm::vec3 &inv_dir, float t_min, float t_max) const;

        glm::vec3 GetDiagonal() const { return __bMax__ - __bMin__; } // 包围盒的对角线

        float GetSurfaceArea() const
        {
            auto diagonal = GetDiagonal();
            return (diagonal.x * (diagonal.y + diagonal.z) + diagonal.y * diagonal.z) * 2.f;
        }

        /*
            8个顶点的对应关系
            idx	 二进制	位运算结果	  获取的顶点坐标
            0	 000    所有位都为0	  (bMin.x, bMin.y, bMin.z)
            1	 001    第0位为1	  (bMax.x, bMin.y, bMin.z)
            2	 010    第1位为1	  (bMin.x, bMax.y, bMin.z)
            3	 011    第0,1位为1	  (bMax.x, bMax.y, bMin.z)
            4	 100    第2位为1	  (bMin.x, bMin.y, bMax.z)
            5	 101    第0,2位为1	  (bMax.x, bMin.y, bMax.z)
            6	 110    第1,2位为1	  (bMin.x, bMax.y, bMax.z)
            7	 111    所有位都为1	  (bMax.x, bMax.y, bMax.z)
        */
        glm::vec3 GetCorner(size_t idx) const
        {
            auto corner = __bMax__;
            if ((idx & 0b1) == 0)
                corner.x = __bMin__.x;
            if ((idx & 0b10) == 0)
                corner.y = __bMin__.y;
            if ((idx & 0b100) == 0)
                corner.z = __bMin__.z;
            return corner;
        }

        // 判断是否为一个退化的Bounds
        bool IsValid() const { return __bMax__.x >= __bMin__.x && __bMax__.y >= __bMin__.y && __bMax__.z >= __bMin__.z; }
    };
}