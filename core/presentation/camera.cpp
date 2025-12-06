#include "camera.hpp"
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

namespace pt
{
    Camera::Camera(Film &film, const glm::vec3 &position, const glm::vec3 &viewpoint, float fovy)
        : mFilm(film), mPosition(position)
    {
        mCameraFromClip = glm::inverse(glm::perspective(
            glm::radians(fovy),
            static_cast<float>(mFilm.GetWidth()) / static_cast<float>(mFilm.GetHeight()),
            1.f, 2.f));
        mWorldFromCamera = glm::inverse(glm::lookAt(mPosition, viewpoint, {0.f, 1.f, 0.f}));
    }

    Ray Camera::GenerateRay(const glm::ivec2 &pixel_coord, const glm::vec2 &offset) const
    {
        glm::vec2 ndc = (glm::vec2(pixel_coord) + offset) / glm::vec2(mFilm.GetWidth(), mFilm.GetHeight());
        ndc.y = 1.f - ndc.y;   // ndc坐标系y轴向上, 而屏幕坐标系y轴向下, 所以需要翻转y轴
        ndc = ndc * 2.f - 1.f; // 将ndc坐标从[0, 1]映射到[-1, 1]范围
        /*
            clip空间坐标(x, y, 0, near) ---> NDC空间坐标(x/near, y/near, 0)
            故设置近平面near为1, 可以用clip坐标xyz直接得到NDC坐标
        */
        glm::vec4 clip_coord = glm::vec4(ndc, 0.f, 1.f);
        glm::vec3 world_coord = mWorldFromCamera * mCameraFromClip * clip_coord;

        return Ray{mPosition, glm::normalize(world_coord - mPosition)};
    }
}