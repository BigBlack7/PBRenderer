#pragma once

#include "film.hpp"
#include "ray.hpp"

namespace pt
{
    class Camera
    {
    private:
        Film &mFilm;
        glm::vec3 mPosition;

        glm::mat4 mCameraFromClip;  // 从裁剪空间到相机空间的变换矩阵
        glm::mat4 mWorldFromCamera; // 从相机空间到世界空间的变换矩阵

    public:
        Camera(Film &film, const glm::vec3 &position, const glm::vec3 &viewpoint, float fovy);
        Ray GenerateRay(const glm::ivec2 &pixel_coord, const glm::vec2 &offset = {0.5, 0.5}) const;

        Film &GetFilm() { return mFilm; }
        const Film &GetFilm() const { return mFilm; }
    };
}
