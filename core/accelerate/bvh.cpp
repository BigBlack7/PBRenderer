#include "bvh.hpp"

namespace pt
{
    void BVH::Build(std::vector<Triangle> &&triangles)
    {
        mRoot = new BVHNode();
        mRoot->__triangles__ = std::move(triangles);
        mRoot->UpdateBounds();
        RecursiveSplitByAxis(mRoot);
    }

    std::optional<HitInfo> BVH::Intersect(const Ray &ray, float t_min, float t_max) const
    {
        std::optional<HitInfo> closest_hit_info;
        RecursiveIntersect(mRoot, ray, t_min, t_max, closest_hit_info);
        return closest_hit_info;
    }

    void BVH::RecursiveSplitByAxis(BVHNode *node)
    {
        if (node->__triangles__.size() == 1)
        {
            return;
        }

        auto diagonal = node->__bounds__.GetDiagonal();
        size_t max_axis = diagonal.x > diagonal.y ? (diagonal.x > diagonal.z ? 0 : 2) : (diagonal.y > diagonal.z ? 1 : 2);
        float mid = node->__bounds__.__bMin__[max_axis] + diagonal[max_axis] * 0.5f;

        std::vector<Triangle> left_triangles, right_triangles;
        for (const auto &triangle : node->__triangles__)
        {
            if ((triangle.__p0__[max_axis] + triangle.__p1__[max_axis] + triangle.__p2__[max_axis]) / 3.f < mid)
            {
                left_triangles.push_back(triangle);
            }
            else
            {
                right_triangles.push_back(triangle);
            }
        }
        if (left_triangles.empty() || right_triangles.empty())
        {
            return;
        }

        auto *left_node = new BVHNode{};
        auto *right_node = new BVHNode{};
        node->__children__[0] = left_node;
        node->__children__[1] = right_node;
        node->__triangles__.clear();
        node->__triangles__.shrink_to_fit();
        left_node->__triangles__ = std::move(left_triangles);
        right_node->__triangles__ = std::move(right_triangles);
        left_node->UpdateBounds();
        right_node->UpdateBounds();
        RecursiveSplitByAxis(left_node);
        RecursiveSplitByAxis(right_node);
    }

    void BVH::RecursiveIntersect(BVHNode *node, const Ray &ray, float t_min, float t_max, std::optional<HitInfo> &closest_hit_info) const
    {
        if (!node->__bounds__.HasIntersection(ray, t_min, t_max))
        {
            return;
        }

        if(node->__triangles__.empty()) // 非叶子节点递归进入子节点
        {
            RecursiveIntersect(node->__children__[0], ray, t_min, t_max, closest_hit_info);
            RecursiveIntersect(node->__children__[1], ray, t_min, t_max, closest_hit_info);
        }
        else // 叶子节点三角形相交检查
        {
            for(const auto &triangle:node->__triangles__)
            {
                auto hit_info=triangle.Intersect(ray, t_min, t_max);
                if(hit_info)
                {
                    t_max = hit_info->__t__;
                    closest_hit_info = hit_info;
                }
            }
        }
    }
}