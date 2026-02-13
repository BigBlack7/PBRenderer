#pragma once
#include "film.hpp"
#include "light/ray.hpp"

namespace pbrt
{
    enum class Direction
    {
        Forward,
        Backward,
        Left,
        Right,
        Up,
        Down
    };

    class Camera
    {
    private:
        Film &mFilm;
        glm::vec3 mPosition;

        glm::mat4 mCameraFromClip;  // 从裁剪空间到相机空间的变换矩阵
        glm::mat4 mWorldFromCamera; // 从相机空间到世界空间的变换矩阵

        // 相机移动
        glm::vec3 mViewDirection;           // 相机朝向
        float mTheta, mPhi;                 // 相机的俯仰角和偏航角
        float mFovy;                        // 相机的视张角
        float mMoveSpeed{2.f};              // 移动速度
        glm::vec2 mTurnSpeed{0.15f, 0.07f}; // 转向速度

    private:
        void Update(); // 更新相机矩阵

    public:
        Camera(Film &film, const glm::vec3 &position, const glm::vec3 &viewpoint, float fovy);
        Ray GenerateRay(const glm::ivec2 &pixel_coord, const glm::vec2 &offset = {0.5f, 0.5f}) const;

        Film &GetFilm() { return mFilm; }
        const Film &GetFilm() const { return mFilm; }

        // 相机移动
        void Move(float dt, Direction direction); // 调整位置
        void Turn(const glm::vec2 &delta);        // 调整朝向, 鼠标移动
        void Zoom(float delta);                   // 调整视角大小, 鼠标滚轮
    };
}