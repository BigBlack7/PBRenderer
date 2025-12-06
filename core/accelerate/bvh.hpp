#pragma once
#include "bounds.hpp"
#include "shape/triangle.hpp"

namespace pt
{
    struct BVHNode
    {
    public:
        Bounds __bounds__{};
        std::vector<Triangle> __triangles__;
        BVHNode *__children__[2];

    public:
        void UpdateBounds()
        {
            __bounds__ = {};
            for (const auto &triangle : __triangles__)
            {
                __bounds__.Expand(triangle.__p0__);
                __bounds__.Expand(triangle.__p1__);
                __bounds__.Expand(triangle.__p2__);
            }
        }
    };

    class BVH : public Shape
    {
    public:
        void Build(std::vector<Triangle> &&triangles);
        std::optional<HitInfo> Intersect(const Ray &ray, float t_min, float t_max) const override;

    private:
        void RecursiveSplitByAxis(BVHNode *node);
        void RecursiveIntersect(BVHNode *node, const Ray &ray, float t_min, float t_max, std::optional<HitInfo> &closest_hit_info) const;
    private:
        BVHNode *mRoot;
    };
}
