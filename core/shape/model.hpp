#pragma once

#include "triangle.hpp"
#include <vector>
#include <filesystem>

namespace pt
{
    class Model : public Shape
    {
    private:
        std::vector<Triangle> mTriangles;

    public:
        Model(const std::vector<Triangle> &triangles) : mTriangles(triangles) {}
        Model(const std::filesystem::path &filename); // 读取obj文件

        std::optional<HitInfo> Intersect(const Ray &ray, float t_min, float t_max) const override;
    };
}
