#include "camera.hpp"
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

namespace pbrt
{
    // 世界空间 -> 相机空间 -> 裁剪空间 -> NDC空间 -> 屏幕空间
    Camera::Camera(Film &film, const glm::vec3 &position, const glm::vec3 &viewpoint, float fovy) : mFilm(film), mPosition(position), mFovy(fovy)
    {
        mViewDirection = glm::normalize(viewpoint - position);
        Update();
        mTheta = glm::degrees(glm::acos(mViewDirection.y));
        if (glm::abs(mViewDirection.y) == 1)
        {
            mPhi = 0.f;
        }
        else
        {
            mPhi = glm::degrees(glm::acos(mViewDirection.x / glm::sqrt(mViewDirection.x * mViewDirection.x + mViewDirection.z * mViewDirection.z)));
        }
    }

    Ray Camera::GenerateRay(const glm::ivec2 &pixel_coord, const glm::vec2 &offset) const
    {
        glm::vec2 ndc = (glm::vec2(pixel_coord) + offset) / glm::vec2(mFilm.GetWidth(), mFilm.GetHeight());
        ndc.y = 1.f - ndc.y;   // ndc坐标系y轴向上, 而屏幕坐标系y轴向下, 所以需要翻转y轴
        ndc = ndc * 2.f - 1.f; // 将ndc坐标从[0, 1]映射到[-1, 1]范围
        /*
            clip空间坐标(x, y, 0, near) ---> NDC空间坐标(x/near, y/near, 0, 1)
            故设置近平面near为1, 可以用NDC坐标直接得到clip坐标xyz
        */
        glm::vec4 clip_coord{ndc, 0.f, 1.f};
        glm::vec3 world_coord = mWorldFromCamera * mCameraFromClip * clip_coord;

        return Ray{mPosition, glm::normalize(world_coord - mPosition)}; // 相机位置为射线原点, 方向为相机位置指向世界坐标
    }

    void Camera::Move(float dt, Direction direction)
    {
        glm::vec3 forward = mViewDirection;
        forward.y = 0; // 忽略y轴的影响
        forward = glm::normalize(forward);
        glm::vec3 moveDirection{};
        switch (direction)
        {
        case Direction::Forward:
            moveDirection = forward;
            break;
        case Direction::Backward:
            moveDirection = -forward;
            break;
        case Direction::Left:
            moveDirection = glm::normalize(glm::cross(forward, glm::vec3{0.f, 1.f, 0.f}));
            break;
        case Direction::Right:
            moveDirection = -glm::normalize(glm::cross(forward, glm::vec3{0.f, 1.f, 0.f}));
            break;
        case Direction::Up:
            moveDirection = glm::vec3{0.f, 1.f, 0.f};
            break;
        case Direction::Down:
            moveDirection = glm::vec3{0.f, -1.f, 0.f};
            break;
        default:
            break;
        }
        mPosition += moveDirection * mMoveSpeed * dt;
        Update();
    }

    void Camera::Turn(const glm::vec2 &delta)
    {
        mPhi -= delta.x * mTurnSpeed.x; // 绕y轴旋转
        if (mPhi > 360.f)
            mPhi -= 360.f;
        if (mPhi < 0.f)
            mPhi += 360.f;
        mTheta += delta.y * mTurnSpeed.y; // 绕x轴旋转
        mTheta = glm::clamp(mTheta, 1.f, 179.f);

        float sin_theta = glm::sin(glm::radians(mTheta));
        float cos_theta = glm::cos(glm::radians(mTheta));
        float sin_phi = glm::sin(glm::radians(mPhi));
        float cos_phi = glm::cos(glm::radians(mPhi));
        mViewDirection = glm::vec3{sin_theta * cos_phi, cos_theta, sin_theta * sin_phi};
        Update();
    }

    void Camera::Zoom(float delta)
    {
        mFovy = glm::clamp(mFovy - delta, 1.f, 179.f);
        Update();
    }

    void Camera::Update()
    {
        auto viewpoint = mPosition + mViewDirection;
        /* 剪裁空间到相机空间的变换矩阵
           相机到剪裁空间需要进行透视投影, 故获得透视矩阵的逆矩阵
         */
        mCameraFromClip = glm::inverse(glm::perspective(
            glm::radians(mFovy),
            static_cast<float>(mFilm.GetWidth()) / static_cast<float>(mFilm.GetHeight()),
            1.f,
            2.f));
        /* 相机空间到世界空间的变换矩阵
           物体从世界坐标到相机坐标需要进行相机的变换, 故获得相机的逆矩阵
        */
        mWorldFromCamera = glm::inverse(glm::lookAt(mPosition, viewpoint, {0.f, 1.f, 0.f}));
    }
}