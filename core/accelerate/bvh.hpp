#pragma once
#include "bounds.hpp"
#include "shape/triangle.hpp"
#include "sampler/aliasTable.hpp"
#include "thread/spinLock.hpp"

namespace pbrt
{
    // BVH二叉树节点
    struct BVHTreeNode
    {
    public:
        Bounds __bounds__{};
        size_t __start__, __end__; // [__start__, __end__)
        BVHTreeNode *__children__[2];

        size_t __depth__;
        uint8_t __splitAxis__;
    };

    // BVH线性节点
    struct alignas(32) BVHNode
    {
    public:
        Bounds __bounds__{};
        union
        {
            int __right__;       // 右子节点索引(仅非叶子节点有效)
            int __triangleIdx__; // 三角形起始位置索引(仅叶子节点有效)
        };
        uint16_t __triangleCount__; // 节点三角形数量
        uint8_t __splitAxis__;
    };

    struct BVHState
    {
    public:
        std::atomic<size_t> __totalNodeCount__{}; // 总节点数
        size_t __leafNodeCount__{};               // 叶子节点数
        size_t __maxLeafNodeTriangleCount__{};    // 最大叶子节点三角形数量
        size_t __maxTreeDepth__{};
        SpinLock __lock__{};

    public:
        void AddLeafNode(BVHTreeNode *node)
        {
            Guard guard(__lock__);
            __leafNodeCount__++;
            __maxLeafNodeTriangleCount__ = glm::max(__maxLeafNodeTriangleCount__, node->__end__ - node->__start__);
            __maxTreeDepth__ = glm::max(__maxTreeDepth__, node->__depth__);
        }
    };

    class BVHTreeNodeAllocator // BVH二叉树节点分配器, 避免频繁申请内存的开销
    {
    private:
        size_t mPtr;
        std::vector<BVHTreeNode *> mNodesList;
        SpinLock mLock{};

    public:
        BVHTreeNodeAllocator() : mPtr(4096) {}

        BVHTreeNode *Allocate()
        {
            Guard guard(mLock);
            // 初始化时会一次性申请大量缓存, 此后只当指针指向最后一个缓存块时, 再申请新的缓存块
            if (mPtr == 4096)
            {
                mNodesList.push_back(new BVHTreeNode[4096]);
                mPtr = 0;
            }
            // 返回当前最后一块缓存(当前可用的节点地址)的指针, 并将指针后移一位
            return &(mNodesList.back()[mPtr++]);
        }

        ~BVHTreeNodeAllocator()
        {
            for (auto *nodes : mNodesList)
            {
                delete[] nodes;
            }
            mNodesList.clear();
        }
    };

    class BVH : public Shape
    {
    public:
        void Build(std::vector<Triangle> &&triangles);
        std::optional<HitInfo> Intersect(const Ray &ray, float t_min, float t_max) const override;
        Bounds GetBounds() const override { return mNodes[0].__bounds__; }
        float GetArea() const override { return mArea; }
        std::optional<ShapeInfo> SampleShape(const RNG &rng) const override;

    private:
        void RecursiveSplit(BVHTreeNode *node, BVHState &state); // 递归划分BVH节点(二叉树结构)
        size_t RecursiveFlatten(BVHTreeNode *node);              // 递归将BVH二叉树转换为线性结构

    private:
        std::vector<BVHNode> mNodes;
        std::vector<Triangle> mOrderedTriangles;
        BVHTreeNodeAllocator mNodeAllocator{};
        BVHTreeNode *mRoot;
        float mArea;
        AliasTable mTable; // 三角形采样表
    };
}