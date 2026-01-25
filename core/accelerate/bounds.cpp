#include "bounds.hpp"

namespace pbrt
{
    bool Bounds::HasIntersection(const Ray &ray, float t_min, float t_max) const
    {
        /*
            P = O + tD
            t = (P - O) / D

                z
                ▲
                │     ┌───────┐
                │    ╱       ╱
                │   ┌───────┐
                │  ╱       ╱
                │ ┌───────┐
                └─────────────▶ y
               ╱
              ╱
             ▼ x

            __bMin__ = (x_min, y_min, z_min)  // 左下后角
            __bMax__ = (x_max, y_max, z_max)  // 右上前角
        */
        glm::vec3 inv_dir = 1.f / ray.__direction__;
        glm::vec3 t1 = (__bMin__ - ray.__origin__) * inv_dir;
        glm::vec3 t2 = (__bMax__ - ray.__origin__) * inv_dir;
        glm::vec3 tmin = glm::min(t1, t2);                       // tmin[i]: 射线在第i轴上的进入参数
        glm::vec3 tmax = glm::max(t1, t2);                       // tmax[i]: 射线在第i轴上的离开参数
        float near = glm::max(tmin.x, glm::max(tmin.y, tmin.z)); // 最晚的进入时间
        float far = glm::min(tmax.x, glm::min(tmax.y, tmax.z));  // 最早的离开时间
        return glm::max(near, t_min) <= glm::min(far, t_max);
    }

    bool Bounds::HasIntersection(const Ray &ray, const glm::vec3 &inv_dir, float t_min, float t_max) const
    {
        glm::vec3 t1 = (__bMin__ - ray.__origin__) * inv_dir;
        glm::vec3 t2 = (__bMax__ - ray.__origin__) * inv_dir;
        glm::vec3 tmin = glm::min(t1, t2);
        glm::vec3 tmax = glm::max(t1, t2);
        float near = glm::max(tmin.x, glm::max(tmin.y, tmin.z));
        float far = glm::min(tmax.x, glm::min(tmax.y, tmax.z));
        return glm::max(near, t_min) <= glm::min(far, t_max);
    }
}