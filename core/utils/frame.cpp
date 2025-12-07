#include "frame.hpp"

namespace pbrt
{
    Frame::Frame(const glm::vec3 &normal)
    {
        mY = glm::normalize(normal);
        glm::vec3 up = glm::abs(normal.y) < 0.99999 ? glm::vec3(0, 1, 0) : glm::vec3(0, 0, 1);
        mX = glm::normalize(glm::cross(up, normal));
        mZ = glm::normalize(glm::cross(mX, mY));
    }

    glm::vec3 Frame::LocalFromWorld(const glm::vec3 &world_dir) const
    {
        return glm::normalize(glm::vec3{glm::dot(world_dir, mX), glm::dot(world_dir, mY), glm::dot(world_dir, mZ)});
    }

    glm::vec3 Frame::WorldFromLocal(const glm::vec3 &local_dir) const
    {
        return glm::normalize(local_dir.x * mX + local_dir.y * mY + local_dir.z * mZ);
    }
}