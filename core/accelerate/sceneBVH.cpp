#include "sceneBVH.hpp"
#include "utils/debugMacro.hpp"
#include "utils/logger.hpp"
#include <array>
namespace pbrt
{
    void SceneBVH::Build(std::vector<ShapeInfo> &&shapeInfos)
    {
        auto *root = mNodeAllocator.Allocate();

        auto shapeInfos_temp = std::move(shapeInfos);
        for (auto &shapeInfo : shapeInfos_temp)
        {
            if (shapeInfo.__shape__.GetBounds().IsValid())
            {
                shapeInfo.UpdateBounds();
                root->__shapeInfos__.push_back(shapeInfo);
            }
            else
            {
                mInfinityShapeInfos.push_back(shapeInfo);
            }
        }

        root->UpdateBounds();
        root->__depth__ = 1;
        SceneBVHState state{};
        size_t shapeInfo_count = root->__shapeInfos__.size();
        RecursiveSplitBySAHB(root, state);

        PBRT_INFO("Total Scene Node Count: {}", state.__totalNodeCount__);
        PBRT_INFO("Scene Leaf Node Count: {}", state.__leafNodeCount__);
        PBRT_INFO("ShapeInfo Count: {}", shapeInfo_count);
        PBRT_INFO("Mean Scene Leaf Node ShapeInfo Count: {}", static_cast<float>(shapeInfo_count) / static_cast<float>(state.__leafNodeCount__));
        PBRT_INFO("Max Scene Leaf Node ShapeInfo Count: {}", state.__maxLeafNodeShapeInfoCount__);
        PBRT_INFO("Max Scene Tree Depth: {}", state.__maxTreeDepth__);

        // 预分配内存
        mNodes.reserve(state.__totalNodeCount__);
        mOrderedShapeInfos.reserve(shapeInfo_count);
        RecursiveFlatten(root);
    }

    std::optional<HitInfo> SceneBVH::Intersect(const Ray &ray, float t_min, float t_max) const
    {
        std::optional<HitInfo> closest_hit_info;
        const ShapeInfo *closest_shapeInfo = nullptr;
        DEBUG_INFO(size_t bounds_test_count = 0)

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

            if (node.__shapeInfoCount__ == 0) // 非叶子节点递归进入子节点
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
                auto shapeInfo_iter = mOrderedShapeInfos.begin() + node.__shapeInfoIdx__;
                for (size_t i = 0; i < node.__shapeInfoCount__; i++)
                {
                    auto ray_object = ray.ObjectFromWorld(shapeInfo_iter->__objectFromWorld__);
                    auto hit_info = shapeInfo_iter->__shape__.Intersect(ray_object, t_min, t_max);
                    DEBUG_INFO(ray.__boundsTestCount__ += ray_object.__boundsTestCount__);
                    DEBUG_INFO(ray.__triangleTestCount__ += ray_object.__triangleTestCount__);
                    if (hit_info)
                    {
                        t_max = hit_info->__t__;
                        closest_hit_info = hit_info;
                        closest_shapeInfo = &(*shapeInfo_iter);
                    }
                    ++shapeInfo_iter;
                }

                if (ptr == stack.begin())
                    break;
                current_node_idx = *(--ptr);
            }
        }

        for (const auto &infinity_shapeInfo : mInfinityShapeInfos)
        {
            auto ray_object = ray.ObjectFromWorld(infinity_shapeInfo.__objectFromWorld__);
            auto hit_info = infinity_shapeInfo.__shape__.Intersect(ray_object, t_min, t_max);
            DEBUG_INFO(ray.__boundsTestCount__ += ray_object.__boundsTestCount__);
            DEBUG_INFO(ray.__triangleTestCount__ += ray_object.__triangleTestCount__);
            if (hit_info)
            {
                t_max = hit_info->__t__;
                closest_hit_info = hit_info;
                closest_shapeInfo = &infinity_shapeInfo;
            }
        }

        if (closest_shapeInfo)
        {
            closest_hit_info->__hitPoint__ = closest_shapeInfo->__worldFromObject__ * glm::vec4(closest_hit_info->__hitPoint__, 1.f);
            closest_hit_info->__normal__ = glm::normalize(glm::vec3(glm::transpose(closest_shapeInfo->__objectFromWorld__) * glm::vec4(closest_hit_info->__normal__, 0.f)));
            closest_hit_info->__material__ = closest_shapeInfo->__material__;
        }

        DEBUG_INFO(ray.__boundsTestCount__ += bounds_test_count)
        return closest_hit_info;
    }

    void SceneBVH::RecursiveSplitBySAHB(SceneBVHTreeNode *node, SceneBVHState &state)
    {
        state.__totalNodeCount__++;
        if (node->__shapeInfos__.size() == 1 || node->__depth__ > 32)
        {
            state.AddLeafNode(node);
            return;
        }

        auto diagonal = node->__bounds__.GetDiagonal();
        float min_cost = std::numeric_limits<float>::infinity();
        size_t min_split_idx = 0;
        Bounds min_left_bounds{}, min_right_bounds{};
        size_t min_left_shapeInfo_count = 0, min_right_shapeInfo_count = 0;
        constexpr size_t bucket_count = 12;
        std::vector<size_t> bucket_shapeInfo_indices[3][bucket_count] = {};
        for (size_t axis = 0; axis < 3; axis++)
        {
            Bounds bucket_bounds[bucket_count] = {};
            size_t bucket_shapeInfo_count[bucket_count] = {};
            size_t shapeInfo_idx = 0;
            for (const auto &shapeInfo : node->__shapeInfos__)
            {
                size_t bucket_idx = glm::clamp<size_t>(glm::floor((shapeInfo.__center__[axis] - node->__bounds__.__bMin__[axis]) * bucket_count / diagonal[axis]), 0, bucket_count - 1);

                bucket_bounds[bucket_idx].Expand(shapeInfo.__bounds__);
                bucket_shapeInfo_count[bucket_idx]++;
                bucket_shapeInfo_indices[axis][bucket_idx].push_back(shapeInfo_idx);
                shapeInfo_idx++;
            }

            Bounds left_bounds = bucket_bounds[0];
            size_t left_shapeInfo_count = bucket_shapeInfo_count[0];
            for (size_t i = 1; i <= bucket_count - 1; i++)
            {
                Bounds right_bounds{};
                size_t right_shapeInfo_count = 0;
                for (size_t j = bucket_count - 1; j >= i; j--)
                {
                    right_bounds.Expand(bucket_bounds[j]);
                    right_shapeInfo_count += bucket_shapeInfo_count[j];
                }
                if (right_shapeInfo_count == 0)
                {
                    break;
                }
                if (left_shapeInfo_count != 0)
                {
                    float cost = left_bounds.GetSurfaceArea() * left_shapeInfo_count + right_bounds.GetSurfaceArea() * right_shapeInfo_count;
                    if (cost < min_cost)
                    {
                        min_cost = cost;
                        node->__splitAxis__ = axis;
                        min_split_idx = i;
                        min_left_bounds = left_bounds;
                        min_right_bounds = right_bounds;
                        min_left_shapeInfo_count = left_shapeInfo_count;
                        min_right_shapeInfo_count = right_shapeInfo_count;
                    }
                }
                left_bounds.Expand(bucket_bounds[i]);
                left_shapeInfo_count += bucket_shapeInfo_count[i];
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

        left->__shapeInfos__.reserve(min_left_shapeInfo_count);
        for (size_t i = 0; i < min_split_idx; i++)
        {
            for (size_t idx : bucket_shapeInfo_indices[node->__splitAxis__][i])
            {
                left->__shapeInfos__.push_back(node->__shapeInfos__[idx]);
            }
        }

        right->__shapeInfos__.reserve(min_right_shapeInfo_count);
        for (size_t i = min_split_idx; i < bucket_count; i++)
        {
            for (size_t idx : bucket_shapeInfo_indices[node->__splitAxis__][i])
            {
                right->__shapeInfos__.push_back(node->__shapeInfos__[idx]);
            }
        }

        node->__shapeInfos__.clear();
        node->__shapeInfos__.shrink_to_fit();
        left->__depth__ = node->__depth__ + 1;
        right->__depth__ = node->__depth__ + 1;
        left->__bounds__ = min_left_bounds;
        right->__bounds__ = min_right_bounds;
        RecursiveSplitBySAHB(left, state);
        RecursiveSplitBySAHB(right, state);
    }

    size_t SceneBVH::RecursiveFlatten(SceneBVHTreeNode *node)
    {
        SceneBVHNode SceneBVH_node{
            node->__bounds__,
            0,
            static_cast<uint16_t>(node->__shapeInfos__.size()),
            static_cast<uint8_t>(node->__splitAxis__)};

        auto idx = mNodes.size();
        mNodes.push_back(SceneBVH_node);
        if (SceneBVH_node.__shapeInfoCount__ == 0)
        {
            RecursiveFlatten(node->__children__[0]);
            mNodes[idx].__right__ = RecursiveFlatten(node->__children__[1]);
        }
        else
        {
            mNodes[idx].__shapeInfoIdx__ = mOrderedShapeInfos.size();
            for (const auto &shapeInfo : node->__shapeInfos__)
            {
                mOrderedShapeInfos.push_back(shapeInfo);
            }
        }
        return idx;
    }
}