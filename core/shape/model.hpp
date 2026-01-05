#pragma once

#include "triangle.hpp"
#include "accelerate/bvh.hpp"
#include <vector>
#include <filesystem>

namespace pbrt
{
    class Model : public Shape
    {
    private:
        BVH mBVH{};

    public:
        Model(const std::vector<Triangle> &triangles)
        {
            auto triangles_copy = triangles;
            mBVH.Build(std::move(triangles_copy));
        }
        Model(const std::filesystem::path &filename, bool byMyself); // 读取obj文件 by myself
        Model(const std::filesystem::path &filename);                // 读取obj文件 by rapidobj

        std::optional<HitInfo> Intersect(const Ray &ray, float t_min, float t_max) const override;
        Bounds GetBounds() const override { return mBVH.GetBounds(); }
        float GetArea() const override { return mBVH.GetArea(); }
        std::optional<ShapeInfo> SampleShape(const RNG &rng) const override { return mBVH.SampleShape(rng); }
    };
}