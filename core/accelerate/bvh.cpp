#include "bvh.hpp"
#include "utils/debugMacro.hpp"
#include "utils/logger.hpp"
#include <array>
namespace pt
{
    void BVH::Build(std::vector<Triangle> &&triangles)
    {
        auto *root = mNodeAllocator.Allocate();
        root->__triangles__ = std::move(triangles);
        root->UpdateBounds();
        root->__depth__ = 1;
        BVHState state{};
        size_t triangle_count = root->__triangles__.size();
        RecursiveSplitBySAHB(root, state);

        PBRT_INFO("Total Node Count: {}", state.__totalNodeCount__);
        PBRT_INFO("Leaf Node Count: {}", state.__leafNodeCount__);
        PBRT_INFO("Triangle Count: {}", triangle_count);
        PBRT_INFO("Mean Leaf Node Triangle Count: {}", static_cast<float>(triangle_count) / static_cast<float>(state.__leafNodeCount__));
        PBRT_INFO("Max Leaf Node Triangle Count: {}", state.__maxLeafNodeTriangleCount__);

        // 预分配内存
        mNodes.reserve(state.__totalNodeCount__);
        mOrderedTriangles.reserve(triangle_count);
        RecursiveFlatten(root);
    }

    std::optional<HitInfo> BVH::Intersect(const Ray &ray, float t_min, float t_max) const
    {
        std::optional<HitInfo> closest_hit_info;

        DEBUG_INFO(size_t bounds_test_count = 0, triangle_test_count = 0)

        glm::bvec3 dir_is_neg = {
            ray.__direction__.x < 0,
            ray.__direction__.y < 0,
            ray.__direction__.z < 0};

        glm::vec3 inv_dir = 1.f / ray.__direction__;

        std::array<int, 32> stack;
        auto ptr = stack.begin();
        size_t current_node_idx = 0;

        while (true)
        {
            auto &node = mNodes[current_node_idx];

            DEBUG_INFO(bounds_test_count++)

            if (!node.__bounds__.HasIntersection(ray, inv_dir, t_min, t_max))
            {
                if (ptr == stack.begin())
                    break;

                current_node_idx = *(--ptr);
                continue;
            }

            if (node.__triangleCount__ == 0) // 非叶子节点递归进入子节点
            {
                // 根据光线方向决定先遍历哪一个节点
                if (dir_is_neg[node.__splitAxis__])
                {
                    *(ptr++) = current_node_idx + 1;
                    current_node_idx = node.__right__;
                }
                else
                {
                    current_node_idx++;        // 左节点
                    *(ptr++) = node.__right__; // 右节点
                }
            }
            else // 叶子节点三角形相交检查
            {
                auto triangle_iter = mOrderedTriangles.begin() + node.__triangleIdx__;

                DEBUG_INFO(triangle_test_count += node.__triangleCount__)

                for (size_t i = 0; i < node.__triangleCount__; i++)
                {
                    auto hit_info = triangle_iter->Intersect(ray, t_min, t_max);
                    ++triangle_iter;
                    if (hit_info)
                    {
                        t_max = hit_info->__t__;
                        closest_hit_info = hit_info;

                        DEBUG_INFO(closest_hit_info->__boundsDepth__ = node.__depth__)
                    }
                }

                if (ptr == stack.begin())
                    break;
                current_node_idx = *(--ptr);
            }
        }

        if (closest_hit_info.has_value())
        {
            DEBUG_INFO(closest_hit_info->__boundsTestCount__ = bounds_test_count)
            DEBUG_INFO(closest_hit_info->__triangleTestCount__ = triangle_test_count)
        }

        return closest_hit_info;
    }

    void BVH::RecursiveSplitByAxis(BVHTreeNode *node, BVHState &state)
    {
        state.__totalNodeCount__++;
        if (node->__triangles__.size() == 1 || node->__depth__ > 32)
        {
            state.AddLeafNode(node);
            return;
        }

        auto diagonal = node->__bounds__.GetDiagonal();
        size_t max_axis = diagonal.x > diagonal.y ? (diagonal.x > diagonal.z ? 0 : 2) : (diagonal.y > diagonal.z ? 1 : 2);
        node->__splitAxis__ = max_axis;
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
            state.AddLeafNode(node);
            return;
        }

        auto *left = mNodeAllocator.Allocate();
        auto *right = mNodeAllocator.Allocate();
        node->__children__[0] = left;
        node->__children__[1] = right;
        node->__triangles__.clear();
        node->__triangles__.shrink_to_fit();
        left->__depth__ = node->__depth__ + 1;
        right->__depth__ = node->__depth__ + 1;
        left->__triangles__ = std::move(left_triangles);
        right->__triangles__ = std::move(right_triangles);
        left->UpdateBounds();
        right->UpdateBounds();
        RecursiveSplitByAxis(left, state);
        RecursiveSplitByAxis(right, state);
    }

    void BVH::RecursiveSplitBySAH(BVHTreeNode *node, BVHState &state)
    {
        state.__totalNodeCount__++;
        if (node->__triangles__.size() == 1 || node->__depth__ > 32)
        {
            state.AddLeafNode(node);
            return;
        }

        auto diagonal = node->__bounds__.GetDiagonal();
        float min_cost = std::numeric_limits<float>::infinity();
        Bounds min_left_bounds{}, min_right_bounds{};
        std::vector<Triangle> left_triangles, right_triangles;
        for (size_t axis = 0; axis < 3; axis++)
        {
            for (size_t i = 0; i < 11; i++)
            {
                float mid = node->__bounds__.__bMin__[axis] + diagonal[axis] * (i + 1.f) / 12.f;
                Bounds left_bounds{}, right_bounds{};
                std::vector<Triangle> left_triangles_temp, right_triangles_temp;
                for (const auto &triangle : node->__triangles__)
                {
                    if ((triangle.__p0__[axis] + triangle.__p1__[axis] + triangle.__p2__[axis]) / 3.f < mid)
                    {
                        left_bounds.Expand(triangle.__p0__);
                        left_bounds.Expand(triangle.__p1__);
                        left_bounds.Expand(triangle.__p2__);
                        left_triangles_temp.push_back(triangle);
                    }
                    else
                    {
                        right_bounds.Expand(triangle.__p0__);
                        right_bounds.Expand(triangle.__p1__);
                        right_bounds.Expand(triangle.__p2__);
                        right_triangles_temp.push_back(triangle);
                    }
                }
                if (left_triangles_temp.empty() || right_triangles_temp.empty())
                {
                    continue;
                }
                float cost = left_bounds.GetSurfaceArea() * left_triangles_temp.size() + right_bounds.GetSurfaceArea() * right_triangles_temp.size();
                if (cost < min_cost)
                {
                    min_cost = cost;
                    left_triangles = std::move(left_triangles_temp);
                    right_triangles = std::move(right_triangles_temp);
                    node->__splitAxis__ = axis;
                    min_left_bounds = left_bounds;
                    min_right_bounds = right_bounds;
                }
            }
        }

        if (left_triangles.empty() || right_triangles.empty())
        {
            state.AddLeafNode(node);
            return;
        }

        auto *left = mNodeAllocator.Allocate();
        auto *right = mNodeAllocator.Allocate();
        node->__children__[0] = left;
        node->__children__[1] = right;
        node->__triangles__.clear();
        node->__triangles__.shrink_to_fit();
        left->__depth__ = node->__depth__ + 1;
        right->__depth__ = node->__depth__ + 1;
        left->__triangles__ = std::move(left_triangles);
        right->__triangles__ = std::move(right_triangles);
        left->__bounds__ = min_left_bounds;
        right->__bounds__ = min_right_bounds;
        RecursiveSplitBySAH(left, state);
        RecursiveSplitBySAH(right, state);
    }

    void BVH::RecursiveSplitBySAHB(BVHTreeNode *node, BVHState &state)
    {
        state.__totalNodeCount__++;
        if (node->__triangles__.size() == 1 || node->__depth__ > 32)
        {
            state.AddLeafNode(node);
            return;
        }

        auto diagonal = node->__bounds__.GetDiagonal();
        float min_cost = std::numeric_limits<float>::infinity();
        size_t min_split_idx = 0;
        Bounds min_left_bounds{}, min_right_bounds{};
        size_t min_left_triangle_count = 0, min_right_triangle_count = 0;
        constexpr size_t bucket_count = 12;
        std::vector<size_t> bucket_triangle_indices[3][bucket_count] = {};
        for (size_t axis = 0; axis < 3; axis++)
        {
            Bounds bucket_bounds[bucket_count] = {};
            size_t bucket_triangle_count[bucket_count] = {};
            size_t triangle_idx = 0;
            for (const auto &triangle : node->__triangles__)
            {
                auto triangle_center = (triangle.__p0__[axis] + triangle.__p1__[axis] + triangle.__p2__[axis]) / 3.f;
                size_t bucket_idx = glm::clamp<size_t>(glm::floor((triangle_center - node->__bounds__.__bMin__[axis]) * bucket_count / diagonal[axis]), 0, bucket_count - 1);
                bucket_bounds[bucket_idx].Expand(triangle.__p0__);
                bucket_bounds[bucket_idx].Expand(triangle.__p1__);
                bucket_bounds[bucket_idx].Expand(triangle.__p2__);
                bucket_triangle_count[bucket_idx]++;
                bucket_triangle_indices[axis][bucket_idx].push_back(triangle_idx);
                triangle_idx++;
            }

            Bounds left_bounds = bucket_bounds[0];
            size_t left_triangle_count = bucket_triangle_count[0];
            for (size_t i = 1; i <= bucket_count - 1; i++)
            {
                Bounds right_bounds{};
                size_t right_triangle_count = 0;
                for (size_t j = bucket_count - 1; j >= i; j--)
                {
                    right_bounds.Expand(bucket_bounds[j]);
                    right_triangle_count += bucket_triangle_count[j];
                }
                if (right_triangle_count == 0)
                {
                    break;
                }
                if (left_triangle_count != 0)
                {
                    float cost = left_bounds.GetSurfaceArea() * left_triangle_count + right_bounds.GetSurfaceArea() * right_triangle_count;
                    if (cost < min_cost)
                    {
                        min_cost = cost;
                        node->__splitAxis__ = axis;
                        min_split_idx = i;
                        min_left_bounds = left_bounds;
                        min_right_bounds = right_bounds;
                        min_left_triangle_count = left_triangle_count;
                        min_right_triangle_count = right_triangle_count;
                    }
                }
                left_bounds.Expand(bucket_bounds[i]);
                left_triangle_count += bucket_triangle_count[i];
            }
        }

        // 没有找到好的分割轴
        if (min_split_idx == 0)
        {
            state.AddLeafNode(node);
            return;
        }

        auto *left = mNodeAllocator.Allocate();
        auto *right = mNodeAllocator.Allocate();
        node->__children__[0] = left;
        node->__children__[1] = right;

        left->__triangles__.reserve(min_left_triangle_count);
        for (size_t i = 0; i < min_split_idx; i++)
        {
            for (size_t idx : bucket_triangle_indices[node->__splitAxis__][i])
            {
                left->__triangles__.push_back(node->__triangles__[idx]);
            }
        }
        
        right->__triangles__.reserve(min_right_triangle_count);
        for (size_t i = min_split_idx; i < bucket_count; i++)
        {
            for (size_t idx : bucket_triangle_indices[node->__splitAxis__][i])
            {
                right->__triangles__.push_back(node->__triangles__[idx]);
            }
        }

        node->__triangles__.clear();
        node->__triangles__.shrink_to_fit();
        left->__depth__ = node->__depth__ + 1;
        right->__depth__ = node->__depth__ + 1;
        left->__bounds__ = min_left_bounds;
        right->__bounds__ = min_right_bounds;
        RecursiveSplitBySAHB(left, state);
        RecursiveSplitBySAHB(right, state);
    }

    size_t BVH::RecursiveFlatten(BVHTreeNode *node)
    {
        BVHNode bvh_node{
            node->__bounds__,
            0,
            static_cast<uint16_t>(node->__triangles__.size()),
            static_cast<uint8_t>(node->__depth__),
            static_cast<uint8_t>(node->__splitAxis__)};

        auto idx = mNodes.size();
        mNodes.push_back(bvh_node);
        if (bvh_node.__triangleCount__ == 0)
        {
            RecursiveFlatten(node->__children__[0]);
            mNodes[idx].__right__ = RecursiveFlatten(node->__children__[1]);
        }
        else
        {
            mNodes[idx].__triangleIdx__ = mOrderedTriangles.size();
            for (const auto &triangle : node->__triangles__)
            {
                mOrderedTriangles.push_back(triangle);
            }
        }
        return idx;
    }
}