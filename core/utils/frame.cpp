#include "frame.hpp"

namespace pbrt
{
    Frame::Frame(const glm::vec3 &normal)
    {
        mY = glm::normalize(normal);
        glm::vec3 up = glm::abs(mY.y) < 0.99999 ? glm::vec3(0, 1, 0) : glm::vec3(0, 0, 1); // 避免法线与默认Up向量平行时的奇异性
        mX = glm::normalize(glm::cross(mY, up));
        mZ = glm::normalize(glm::cross(mX, mY));
    }

    glm::vec3 Frame::LocalFromWorld(const glm::vec3 &world_dir) const
    {
        return glm::normalize(glm::vec3{glm::dot(world_dir, mX), glm::dot(world_dir, mY), glm::dot(world_dir, mZ)});
    }

    /*
        v_world = M · v_local

        v_world = | mX_x  mY_x  mZ_x |   | x_local |
                  | mX_y  mY_y  mZ_y | · | y_local |
                  | mX_z  mY_z  mZ_z |   | z_local |
    */
    glm::vec3 Frame::WorldFromLocal(const glm::vec3 &local_dir) const
    {
        return glm::normalize(local_dir.x * mX + local_dir.y * mY + local_dir.z * mZ);
    }
}