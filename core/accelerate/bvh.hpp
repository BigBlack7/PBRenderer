#pragma once
#include "bounds.hpp"
#include "shape/triangle.hpp"
#include "sampler/aliasTable.hpp"
#include "thread/spinLock.hpp"

namespace pbrt
{
    struct BVHTreeNode
    {
    public:
        Bounds __bounds__{};
        size_t __start__, __end__;
        BVHTreeNode *__children__[2];

        size_t __depth__;
        uint8_t __splitAxis__;
    };

    struct alignas(32) BVHNode
    {
    public:
        Bounds __bounds__{};
        union
        {
            int __right__;
            int __triangleIdx__;
        };
        uint16_t __triangleCount__;
        uint8_t __splitAxis__;
    };

    struct BVHState
    {
    public:
        std::atomic<size_t> __totalNodeCount__{};
        size_t __leafNodeCount__{};
        size_t __maxLeafNodeTriangleCount__{};
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

    class BVHTreeNodeAllocator
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
            if (mPtr == 4096)
            {
                mNodesList.push_back(new BVHTreeNode[4096]);
                mPtr = 0;
            }
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
        void RecursiveSplit(BVHTreeNode *node, BVHState &state);
        size_t RecursiveFlatten(BVHTreeNode *node);

    private:
        std::vector<BVHNode> mNodes;
        std::vector<Triangle> mOrderedTriangles;
        BVHTreeNodeAllocator mNodeAllocator{};
        BVHTreeNode *mRoot;
        float mArea;
        AliasTable mTable;
    };
}