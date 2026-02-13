#include "sceneBVH.hpp"
#include "utils/debugMacro.hpp"
#include "utils/logger.hpp"
#include <array>

namespace pbrt
{
    void SceneBVH::Build(std::vector<ShapeBVHInfo> &&shapeBVHInfos)
    {
        auto shapeBVHInfos_temp = std::move(shapeBVHInfos);
        for (auto &shapeBVHInfo : shapeBVHInfos_temp)
        {
            if (shapeBVHInfo.__shape__->GetBounds().IsValid())
            {
                shapeBVHInfo.UpdateBounds();
                mOrderedShapeBVHInfos.push_back(shapeBVHInfo);
            }
            else
            {
                mInfinityShapeBVHInfos.push_back(shapeBVHInfo);
            }
        }

        mRoot = mNodeAllocator.Allocate();
        mRoot->__start__ = 0;
        mRoot->__end__ = mOrderedShapeBVHInfos.size();
        mRoot->__bounds__ = {};
        for (const auto &shapeBVHInfo : mOrderedShapeBVHInfos)
        {
            mRoot->__bounds__.Expand(shapeBVHInfo.__bounds__);
        }
        mRoot->__depth__ = 1;

        SceneBVHState state{};
        size_t shapeBVHInfo_count = mOrderedShapeBVHInfos.size();
        RecursiveSplit(mRoot, state);
        MasterThreadPool.Wait();

        PBRT_INFO("--Scene BVH State--");
        PBRT_DEBUG("Scene - Total Node Count: {}", (size_t)state.__totalNodeCount__);
        PBRT_DEBUG("Scene - Leaf Node Count: {}", state.__leafNodeCount__);
        PBRT_DEBUG("Scene - ShapeBVHInfo Count: {}", shapeBVHInfo_count);
        PBRT_DEBUG("Scene - Mean Leaf Node ShapeBVHInfo Count: {}", static_cast<float>(shapeBVHInfo_count) / static_cast<float>(state.__leafNodeCount__));
        PBRT_DEBUG("Scene - Max Leaf Node ShapeBVHInfo Count: {}", state.__maxLeafNodeShapeBVHInfoCount__);
        PBRT_DEBUG("Scene - Max Tree Depth: {}", state.__maxTreeDepth__);

        // 预分配内存
        mNodes.reserve(state.__totalNodeCount__);
        RecursiveFlatten(mRoot);
    }

    std::optional<HitInfo> SceneBVH::Intersect(const Ray &ray, float t_min, float t_max) const
    {
        std::optional<HitInfo> closest_hit_info;
        const ShapeBVHInfo *closest_shapeBVHInfo = nullptr;
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

            if (node.__shapeBVHInfoCount__ == 0) // 非叶子节点递归进入子节点
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
                auto shapeBVHInfo_iter = mOrderedShapeBVHInfos.begin() + node.__shapeBVHInfoIdx__;
                for (size_t i = 0; i < node.__shapeBVHInfoCount__; i++)
                {
                    // 用对象空间光线进行相交检测
                    auto ray_object = ray.ObjectFromWorld(shapeBVHInfo_iter->__objectFromWorld__);
                    auto hit_info = shapeBVHInfo_iter->__shape__->Intersect(ray_object, t_min, t_max);
                    DEBUG_INFO(ray.__boundsTestCount__ += ray_object.__boundsTestCount__);
                    DEBUG_INFO(ray.__triangleTestCount__ += ray_object.__triangleTestCount__);
                    if (hit_info)
                    {
                        t_max = hit_info->__t__;
                        closest_hit_info = hit_info;
                        closest_shapeBVHInfo = &(*shapeBVHInfo_iter);
                    }
                    ++shapeBVHInfo_iter;
                }

                if (ptr == stack.begin())
                    break;
                current_node_idx = *(--ptr);
            }
        }

        for (const auto &infinity_shapeBVHInfo : mInfinityShapeBVHInfos)
        {
            auto ray_object = ray.ObjectFromWorld(infinity_shapeBVHInfo.__objectFromWorld__);
            auto hit_info = infinity_shapeBVHInfo.__shape__->Intersect(ray_object, t_min, t_max);
            DEBUG_INFO(ray.__boundsTestCount__ += ray_object.__boundsTestCount__);
            DEBUG_INFO(ray.__triangleTestCount__ += ray_object.__triangleTestCount__);
            if (hit_info)
            {
                t_max = hit_info->__t__;
                closest_hit_info = hit_info;
                closest_shapeBVHInfo = &infinity_shapeBVHInfo;
            }
        }

        if (closest_shapeBVHInfo)
        {
            closest_hit_info->__hitPoint__ = closest_shapeBVHInfo->__worldFromObject__ * glm::vec4(closest_hit_info->__hitPoint__, 1.f);
            // TBN方法计算法线变换
            closest_hit_info->__normal__ = glm::normalize(glm::vec3(glm::transpose(closest_shapeBVHInfo->__objectFromWorld__) * glm::vec4(closest_hit_info->__normal__, 0.f)));
            closest_hit_info->__material__ = closest_shapeBVHInfo->__material__;
        }

        DEBUG_INFO(ray.__boundsTestCount__ += bounds_test_count)
        return closest_hit_info;
    }

    void SceneBVH::RecursiveSplit(SceneBVHTreeNode *node, SceneBVHState &state)
    {
        state.__totalNodeCount__++;
        if ((node->__end__ - node->__start__ == 1) || (node->__depth__ > 32))
        {
            state.AddLeafNode(node);
            return;
        }

        auto diagonal = node->__bounds__.GetDiagonal();
        float min_cost = std::numeric_limits<float>::infinity();
        size_t min_split_idx = 0;
        Bounds min_left_bounds{}, min_right_bounds{};
        constexpr size_t bucket_count = 12;
        for (size_t axis = 0; axis < 3; axis++)
        {
            Bounds bucket_bounds[bucket_count] = {};
            size_t bucket_shapeBVHInfo_count[bucket_count] = {};
            for (size_t shapeBVHInfo_idx = node->__start__; shapeBVHInfo_idx < node->__end__; shapeBVHInfo_idx++)
            {
                const auto &shapeBVHInfo = mOrderedShapeBVHInfos[shapeBVHInfo_idx];
                size_t bucket_idx = glm::clamp<size_t>(glm::floor((shapeBVHInfo.__center__[axis] - node->__bounds__.__bMin__[axis]) * bucket_count / diagonal[axis]), 0, bucket_count - 1);
                bucket_bounds[bucket_idx].Expand(shapeBVHInfo.__bounds__);
                bucket_shapeBVHInfo_count[bucket_idx]++;
            }

            Bounds left_bounds = bucket_bounds[0];
            size_t left_shapeBVHInfo_count = bucket_shapeBVHInfo_count[0];
            for (size_t i = 1; i <= bucket_count - 1; i++)
            {
                Bounds right_bounds{};
                size_t right_shapeBVHInfo_count = 0;
                for (size_t j = bucket_count - 1; j >= i; j--)
                {
                    right_bounds.Expand(bucket_bounds[j]);
                    right_shapeBVHInfo_count += bucket_shapeBVHInfo_count[j];
                }
                if (right_shapeBVHInfo_count == 0)
                {
                    break;
                }
                if (left_shapeBVHInfo_count != 0)
                {
                    float cost = left_bounds.GetSurfaceArea() * left_shapeBVHInfo_count + right_bounds.GetSurfaceArea() * right_shapeBVHInfo_count;
                    if (cost < min_cost)
                    {
                        min_cost = cost;
                        node->__splitAxis__ = axis;
                        min_split_idx = i;
                        min_left_bounds = left_bounds;
                        min_right_bounds = right_bounds;
                    }
                }
                left_bounds.Expand(bucket_bounds[i]);
                left_shapeBVHInfo_count += bucket_shapeBVHInfo_count[i];
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

        size_t head_ptr = node->__start__;
        size_t tail_ptr = node->__end__ - 1;
        while (head_ptr <= tail_ptr)
        {
            const auto &shapeBVHInfo_head = mOrderedShapeBVHInfos[head_ptr];
            size_t head_bucket_idx = glm::clamp<size_t>(glm::floor((shapeBVHInfo_head.__center__[node->__splitAxis__] - node->__bounds__.__bMin__[node->__splitAxis__]) * bucket_count / diagonal[node->__splitAxis__]), 0, bucket_count - 1);
            bool head_is_left = head_bucket_idx < min_split_idx;

            const auto &shapeBVHInfo_tail = mOrderedShapeBVHInfos[tail_ptr];
            size_t tail_bucket_idx = glm::clamp<size_t>(glm::floor((shapeBVHInfo_tail.__center__[node->__splitAxis__] - node->__bounds__.__bMin__[node->__splitAxis__]) * bucket_count / diagonal[node->__splitAxis__]), 0, bucket_count - 1);
            bool tail_is_left = tail_bucket_idx < min_split_idx;

            if (head_is_left && tail_is_left)
            {
                head_ptr++;
            }
            else if ((!head_is_left) && (!tail_is_left))
            {
                tail_ptr--;
            }
            else if ((!head_is_left) && tail_is_left)
            {
                std::swap(mOrderedShapeBVHInfos[head_ptr], mOrderedShapeBVHInfos[tail_ptr]);
                tail_ptr--;
                head_ptr++;
            }
            else
            {
                tail_ptr--;
                head_ptr++;
            }
        }
        left->__start__ = node->__start__;
        left->__end__ = head_ptr;
        right->__start__ = left->__end__;
        right->__end__ = node->__end__;
        node->__end__ = node->__start__;

        left->__depth__ = node->__depth__ + 1;
        right->__depth__ = node->__depth__ + 1;
        left->__bounds__ = min_left_bounds;
        right->__bounds__ = min_right_bounds;

        if ((right->__end__ - left->__start__) > (128 * 1024))
        {
            MasterThreadPool.ParallelFor(2, 1, [&, left, right](size_t i, size_t)
                                         {
                                             if (i == 0)
                                             {
                                                 RecursiveSplit(left, state);
                                             }
                                             else
                                             {
                                                 RecursiveSplit(right, state);
                                             }
                                             // end
                                         });
        }
        else
        {
            RecursiveSplit(left, state);
            RecursiveSplit(right, state);
        }
    }

    size_t SceneBVH::RecursiveFlatten(SceneBVHTreeNode *node)
    {
        SceneBVHNode SceneBVH_node{
            node->__bounds__,
            0,
            static_cast<uint16_t>(node->__end__ - node->__start__),
            static_cast<uint8_t>(node->__splitAxis__)};

        auto idx = mNodes.size();
        mNodes.push_back(SceneBVH_node);
        if (SceneBVH_node.__shapeBVHInfoCount__ == 0)
        {
            RecursiveFlatten(node->__children__[0]);
            mNodes[idx].__right__ = RecursiveFlatten(node->__children__[1]);
        }
        else
        {
            mNodes[idx].__shapeBVHInfoIdx__ = node->__start__;
        }
        return idx;
    }
}