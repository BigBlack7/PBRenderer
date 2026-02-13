#include "bvh.hpp"
#include "thread/threadPool.hpp"
#include "utils/debugMacro.hpp"
#include "utils/logger.hpp"
#include <array>

namespace pbrt
{
    void BVH::Build(std::vector<Triangle> &&triangles)
    {
        mOrderedTriangles = std::move(triangles);

        mRoot = mNodeAllocator.Allocate(); // 分配根节点
        // 初始化根节点首尾索引
        mRoot->__start__ = 0;
        mRoot->__end__ = mOrderedTriangles.size();
        // 初始化根节点包围盒
        mRoot->__bounds__ = {};
        for (const auto &triangle : mOrderedTriangles)
        {
            mRoot->__bounds__.Expand(triangle.GetBounds());
        }
        mRoot->__depth__ = 1;

        BVHState state{};
        size_t triangle_count = mOrderedTriangles.size();
        RecursiveSplit(mRoot, state); // 递归构建BVH树
        MasterThreadPool.Wait();

        PBRT_DEBUG("BVH - Total Node Count: {}", (size_t)state.__totalNodeCount__);
        PBRT_DEBUG("BVH - Leaf Node Count: {}", state.__leafNodeCount__);
        PBRT_DEBUG("BVH - Triangle Count: {}", triangle_count);
        PBRT_DEBUG("BVH - Mean Leaf Node Triangle Count: {}", static_cast<float>(triangle_count) / static_cast<float>(state.__leafNodeCount__));
        PBRT_DEBUG("BVH - Max Leaf Node Triangle Count: {}", state.__maxLeafNodeTriangleCount__);
        PBRT_DEBUG("BVH - Max Tree Depth: {}", state.__maxTreeDepth__);

        mNodes.reserve(state.__totalNodeCount__); // 预分配内存
        RecursiveFlatten(mRoot);                  // 递归将BVH树转换为线性结构

        // 以三角形面积为权重构建别名表
        mArea = 0.f;
        std::vector<float> areas;
        areas.reserve(mNodes.size());
        for (const auto &triangle : mOrderedTriangles)
        {
            areas.push_back(triangle.GetArea());
            mArea += areas.back();
        }
        mTable.Build(areas);
    }

    std::optional<HitInfo> BVH::Intersect(const Ray &ray, float t_min, float t_max) const
    {
        std::optional<HitInfo> closest_hit_info;

        DEBUG_INFO(size_t bounds_test_count = 0, triangle_test_count = 0)

        /*
            确认光线方向, 用于确定先遍历哪一个节点
            例如光线x分量为负, 则光线从X轴正半轴射向负半轴, 即从右向左穿入包围盒
            右子节点包围盒中心坐标一定大于左边, 而当光线从右向左穿入包围盒, 相交检测仍会先访问左节点, 随后访问右节点时若命中其包围盒的三角形, 那么之前与左节点的相交测试无意义
        */
        glm::bvec3 dir_is_neg = {
            ray.__direction__.x < 0,
            ray.__direction__.y < 0,
            ray.__direction__.z < 0};

        glm::vec3 inv_dir = 1.f / ray.__direction__;

        std::array<int, 32> stack; // 栈式非递归遍历, 用于存储待访问的节点索引
        auto ptr = stack.begin();
        size_t current_node_idx = 0; // 当前遍历的节点索引

        while (true)
        {
            auto &node = mNodes[current_node_idx];

            DEBUG_INFO(bounds_test_count++)

            // 当前节点包围盒与射线无交, 跳过整个子树(大规模剪枝)
            if (!node.__bounds__.HasIntersection(ray, inv_dir, t_min, t_max))
            {
                // 栈为空, 遍历完成
                if (ptr == stack.begin())
                    break;

                // 从栈中取出下一个要访问的节点
                current_node_idx = *(--ptr);
                continue;
            }

            if (node.__triangleCount__ == 0) // 非叶子节点
            {
                // 根据光线方向决定先遍历哪一个节点
                if (dir_is_neg[node.__splitAxis__])
                {
                    *(ptr++) = current_node_idx + 1;   // 左节点入栈
                    current_node_idx = node.__right__; // 先访问右节点
                }
                else
                {
                    current_node_idx++;        // 先访问左节点
                    *(ptr++) = node.__right__; // 右节点入栈
                }
            }
            else // 叶子节点三角形相交检查
            {
                auto triangle_iter = mOrderedTriangles.begin() + node.__triangleIdx__; // 定位叶节点三角形起始位置

                DEBUG_INFO(triangle_test_count += node.__triangleCount__)

                // 遍历叶子节点内所有三角形进行相交检测
                for (size_t i = 0; i < node.__triangleCount__; i++)
                {
                    auto hit_info = triangle_iter->Intersect(ray, t_min, t_max);
                    ++triangle_iter;
                    if (hit_info)
                    {
                        t_max = hit_info->__t__;
                        closest_hit_info = hit_info;
                    }
                }

                // 栈为空即遍历完成, 否则从栈中取出下一个要访问的节点
                if (ptr == stack.begin())
                    break;
                current_node_idx = *(--ptr);
            }
        }

        DEBUG_INFO(ray.__boundsTestCount__ += bounds_test_count)
        DEBUG_INFO(ray.__triangleTestCount__ += triangle_test_count)

        return closest_hit_info;
    }

    std::optional<ShapeInfo> BVH::SampleShape(const RNG &rng) const
    {
        auto sample_result = mTable.Sample(rng.Uniform());
        const auto &triangle = mOrderedTriangles[sample_result.__idx__];
        auto triangle_sample = triangle.SampleShape(rng);
        if (!triangle_sample.has_value())
        {
            return std::nullopt;
        }
        return ShapeInfo{
            .__point__ = triangle_sample->__point__,
            .__normal__ = triangle_sample->__normal__,
            .__pdf__ = triangle_sample->__pdf__ * sample_result.__prob__
            // end
        };
    }

    /* BVH Build optimization versions:
    void BVH::RecursiveSplitByAxis(BVHTreeNode *node, BVHState &state) // 以最长轴分割节点, 划分不平均, 叶节点三角形数量会很多
    {
        state.__totalNodeCount__++;
        if (node->__triangles__.size() == 1 || node->__depth__ > 32)
        {
            state.AddLeafNode(node);
            return;
        }

        auto diagonal = node->__bounds__.GetDiagonal();
        size_t max_axis = diagonal.x > diagonal.y ? (diagonal.x > diagonal.z ? 0 : 2) : (diagonal.y > diagonal.z ? 1 : 2); // 最长轴
        node->__splitAxis__ = max_axis; //记录划分轴
        float mid = node->__bounds__.__bMin__[max_axis] + diagonal[max_axis] * 0.5f; // 最长轴中点作为分割点

        // 根据三角形中心点位置划分左右子节点
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
            // 左右子节点三角形列表有一个为空, 说明该节点三角形都在同侧, 即不能再分割, 认为该节点为叶子节点
            state.AddLeafNode(node);
            return;
        }

        // 申请左右子节点内存
        auto *left = mNodeAllocator.Allocate();
        auto *right = mNodeAllocator.Allocate();

        // 设置子节点指针
        node->__children__[0] = left;
        node->__children__[1] = right;

        // 清空当前节点三角形列表
        node->__triangles__.clear();
        node->__triangles__.shrink_to_fit();

        // 设置子节点深度和三角形列表
        left->__depth__ = node->__depth__ + 1;
        right->__depth__ = node->__depth__ + 1;
        left->__triangles__ = std::move(left_triangles);
        right->__triangles__ = std::move(right_triangles);

        // 更新子节点包围盒
        left->UpdateBounds();
        right->UpdateBounds();

        // 递归分割子节点
        RecursiveSplitByAxis(left, state);
        RecursiveSplitByAxis(right, state);
    }

    // 基于表面积启发式的递归分割BVH节点
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
        for (size_t axis = 0; axis < 3; axis++) // 遍历三个轴寻得最优划分轴
        {
            for (size_t i = 0; i < 11; i++) // 遍历12个划分点寻得最优划分点
            {
                float mid = node->__bounds__.__bMin__[axis] + diagonal[axis] * (i + 1.f) / 12.f; // 划分点的中心位置
                Bounds left_bounds{}, right_bounds{}; // 左右子节点的包围盒
                std::vector<Triangle> left_triangles_temp, right_triangles_temp;
                for (const auto &triangle : node->__triangles__)
                {
                    // 根据三角形中心点位置划分左右子节点
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

                // 计算当前划分的成本
                // cost = traversal_cost + Prob_left * Σ(T_left_triangles) + Prob_right * Σ(T_right_triangles)
                // Prob_left与Prob_right与包围盒表面积成正比, 这里近似认为traversal_cost是常数项
                // T_left_triangles与T_right_triangles为遍历包围盒内几何体相交测试的时间开销
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

    // 桶式划分三角形加速SAH的构建, 但由于申请内存次数过多, 开销同样较大
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
        std::vector<size_t> bucket_triangle_indices[3][bucket_count] = {}; // 每个桶内的三角形索引
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
                bucket_triangle_indices[axis][bucket_idx].push_back(triangle_idx); // 当前划分轴和划分索引下, 该桶内的三角形索引
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
        for (size_t i = 0; i < min_split_idx; i++) // 0 ~ min_split_idx - 1为左子节点
        {
            for (size_t idx : bucket_triangle_indices[node->__splitAxis__][i])
            {
                left->__triangles__.push_back(node->__triangles__[idx]);
            }
        }

        right->__triangles__.reserve(min_right_triangle_count);
        for (size_t i = min_split_idx; i < bucket_count; i++) // min_split_idx ~ bucket_count - 1为右子节点
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
    */

    void BVH::RecursiveSplit(BVHTreeNode *node, BVHState &state)
    {
        state.__totalNodeCount__++;
        if ((node->__end__ - node->__start__ == 1) || (node->__depth__ > 32))
        {
            // 叶字节点或深度超过32层, 直接添加为叶子节点
            state.AddLeafNode(node);
            return;
        }

        auto diagonal = node->__bounds__.GetDiagonal();
        float min_cost = std::numeric_limits<float>::infinity();
        size_t min_split_idx = 0;
        Bounds min_left_bounds{}, min_right_bounds{};
        constexpr size_t bucket_count = 12;     // 桶的数量
        for (size_t axis = 0; axis < 3; axis++) // 遍历3个轴(X, Y, Z)
        {
            Bounds bucket_bounds[bucket_count] = {};         // 桶的包围盒
            size_t bucket_triangle_count[bucket_count] = {}; // 桶内三角形数量
            // 遍历节点内三角形, 并根据三角形中心点位置将其分配到不同的桶中
            for (size_t triangle_idx = node->__start__; triangle_idx < node->__end__; triangle_idx++)
            {
                Triangle triangle = mOrderedTriangles[triangle_idx];
                float triangle_center = (triangle.__p0__[axis] + triangle.__p1__[axis] + triangle.__p2__[axis]) / 3.f; // 三角形中心点位置
                // 根据三角形中心点位置确定其所属的桶索引, 并将三角形分配到该桶中
                size_t bucket_idx = glm::clamp<size_t>(glm::floor((triangle_center - node->__bounds__.__bMin__[axis]) * bucket_count / diagonal[axis]), 0, bucket_count - 1);
                bucket_bounds[bucket_idx].Expand(triangle.__p0__);
                bucket_bounds[bucket_idx].Expand(triangle.__p1__);
                bucket_bounds[bucket_idx].Expand(triangle.__p2__);
                bucket_triangle_count[bucket_idx]++;
            }

            // 初始以第一个桶为左子节点, 其他桶为右子节点
            Bounds left_bounds = bucket_bounds[0];
            size_t left_triangle_count = bucket_triangle_count[0];
            for (size_t i = 1; i <= bucket_count - 1; i++)
            {
                Bounds right_bounds{};
                size_t right_triangle_count = 0;
                for (size_t j = bucket_count - 1; j >= i; j--) // 从后往前遍历桶, 更新右子节点的包围盒
                {
                    right_bounds.Expand(bucket_bounds[j]);
                    right_triangle_count += bucket_triangle_count[j];
                }

                // 右子节点为空, 则当前划分轴不合适
                if (right_triangle_count == 0)
                {
                    break;
                }

                if (left_triangle_count != 0)
                {
                    // 计算当前划分的成本
                    // cost = traversal_cost + Prob_left * Σ(T_left_triangles) + Prob_right * Σ(T_right_triangles)
                    // Prob_left与Prob_right与包围盒表面积成正比, 这里近似认为traversal_cost是常数项
                    // T_left_triangles与T_right_triangles为遍历包围盒内几何体相交测试的时间开销
                    float cost = left_bounds.GetSurfaceArea() * left_triangle_count + right_bounds.GetSurfaceArea() * right_triangle_count;
                    if (cost < min_cost)
                    {
                        min_cost = cost;
                        node->__splitAxis__ = axis; // 记录最优划分轴
                        min_split_idx = i;          // 记录最优划分索引
                        // 记录最优划分位置的左右子节点包围盒
                        min_left_bounds = left_bounds;
                        min_right_bounds = right_bounds;
                    }
                }

                // 拓展左子节点以计算下一个划分位置成本
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

        // 申请左右子节点内存
        auto *left = mNodeAllocator.Allocate();
        auto *right = mNodeAllocator.Allocate();
        node->__children__[0] = left;
        node->__children__[1] = right;

        // 对当前节点内三角形进行排序并分配到左右子节点(只记录三角形索引, 避免频繁申请内存)
        size_t head_ptr = node->__start__;
        size_t tail_ptr = node->__end__ - 1;
        while (head_ptr <= tail_ptr)
        {
            // 确认头指针指向的三角形是否属于左子节点
            Triangle triangle_head = mOrderedTriangles[head_ptr];
            auto triangle_head_center = (triangle_head.__p0__[node->__splitAxis__] + triangle_head.__p1__[node->__splitAxis__] + triangle_head.__p2__[node->__splitAxis__]) / 3.f;
            size_t head_bucket_idx = glm::clamp<size_t>(glm::floor((triangle_head_center - node->__bounds__.__bMin__[node->__splitAxis__]) * bucket_count / diagonal[node->__splitAxis__]), 0, bucket_count - 1);
            bool head_is_left = head_bucket_idx < min_split_idx;

            // 确认尾指针指向的三角形是否属于左子节点
            Triangle triangle_tail = mOrderedTriangles[tail_ptr];
            auto triangle_tail_center = (triangle_tail.__p0__[node->__splitAxis__] + triangle_tail.__p1__[node->__splitAxis__] + triangle_tail.__p2__[node->__splitAxis__]) / 3.f;
            size_t tail_bucket_idx = glm::clamp<size_t>(glm::floor((triangle_tail_center - node->__bounds__.__bMin__[node->__splitAxis__]) * bucket_count / diagonal[node->__splitAxis__]), 0, bucket_count - 1);
            bool tail_is_left = tail_bucket_idx < min_split_idx;

            if (head_is_left && tail_is_left) // 都在左边
            {
                head_ptr++;
            }
            else if ((!head_is_left) && (!tail_is_left)) // 都在右边
            {
                tail_ptr--;
            }
            else if ((!head_is_left) && tail_is_left) // 头在右边, 尾在左边
            {
                std::swap(mOrderedTriangles[head_ptr], mOrderedTriangles[tail_ptr]);
                tail_ptr--;
                head_ptr++;
            }
            else // 头在左边, 尾在右边
            {
                tail_ptr--;
                head_ptr++;
            }
        }
        // 完成后head_ptr为右子节点的起始索引
        left->__start__ = node->__start__;
        left->__end__ = head_ptr;
        right->__start__ = left->__end__;
        right->__end__ = node->__end__;
        node->__end__ = node->__start__; // 表明当前节点不再包含三角形, 为非叶子节点

        // 更新左右子节点的深度与包围盒
        left->__depth__ = node->__depth__ + 1;
        right->__depth__ = node->__depth__ + 1;
        left->__bounds__ = min_left_bounds;
        right->__bounds__ = min_right_bounds;

        // 大规模三角形划分时并行递归构建子节点
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

    /*
        深度优先-递归地将BVH二叉树节点flatten到一个线性数组中
        对于一个非叶子节点的下一个节点一定是它的左子节点, 然后存储右子节点索引即可
        可以提升相交检测时遍历BVH结构的缓存命中率
    */
    size_t BVH::RecursiveFlatten(BVHTreeNode *node)
    {
        BVHNode bvh_node{
            node->__bounds__,
            0,
            static_cast<uint16_t>(node->__end__ - node->__start__),
            static_cast<uint8_t>(node->__splitAxis__)
            // end
        };

        auto idx = mNodes.size();
        mNodes.push_back(bvh_node);
        if (bvh_node.__triangleCount__ == 0) // 非叶子节点
        {
            RecursiveFlatten(node->__children__[0]);                         // 左子节点紧随其后
            mNodes[idx].__right__ = RecursiveFlatten(node->__children__[1]); // 记录右子节点索引
        }
        else // 叶子节点
        {
            // 存储第一个三角形索引
            mNodes[idx].__triangleIdx__ = node->__start__;
        }
        return idx;
    }
}