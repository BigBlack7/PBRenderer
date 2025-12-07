#pragma once
#include "bounds.hpp"
#include "shape/shape.hpp"

namespace pbrt
{
    struct ShapeInfo
    {
    public:
        const Shape &__shape__;
        const Material *__material__;
        glm::mat4 __worldFromObject__;
        glm::mat4 __objectFromWorld__;
        Bounds __bounds__{};
        glm::vec3 __center__;

    public:
        void UpdateBounds()
        {
            __bounds__ = {};
            auto bounds_object = __shape__.GetBounds();
            for (size_t idx = 0; idx < 8; idx++)
            {
                auto corner_object = bounds_object.GetCorner(idx);
                glm::vec3 corner_world = __worldFromObject__ * glm::vec4(corner_object, 1.f);
                __bounds__.Expand(corner_world);
            }
            __center__ = (__bounds__.__bMax__ + __bounds__.__bMin__) * 0.5f;
        }
    };

    struct SceneBVHTreeNode
    {
    public:
        Bounds __bounds__{};
        std::vector<ShapeInfo> __shapeInfos__;
        SceneBVHTreeNode *__children__[2];

        size_t __depth__;
        uint8_t __splitAxis__;

    public:
        void UpdateBounds()
        {
            __bounds__ = {};
            for (const auto &shapeInfo : __shapeInfos__)
            {
                __bounds__.Expand(shapeInfo.__bounds__);
            }
        }
    };

    struct alignas(32) SceneBVHNode
    {
    public:
        Bounds __bounds__{};
        union
        {
            int __right__;
            int __shapeInfoIdx__;
        };
        uint16_t __shapeInfoCount__;
        uint8_t __splitAxis__;
    };

    struct SceneBVHState
    {
    public:
        size_t __totalNodeCount__{};
        size_t __leafNodeCount__{};
        size_t __maxLeafNodeShapeInfoCount__{};
        size_t __maxTreeDepth__{};

    public:
        void AddLeafNode(SceneBVHTreeNode *node)
        {
            __leafNodeCount__++;
            __maxLeafNodeShapeInfoCount__ = glm::max(__maxLeafNodeShapeInfoCount__, node->__shapeInfos__.size());
            __maxTreeDepth__ = glm::max(__maxTreeDepth__, node->__depth__);
        }
    };

    class SceneBVHTreeNodeAllocator
    {
    private:
        size_t mPtr;
        std::vector<SceneBVHTreeNode *> mNodesList;

    public:
        SceneBVHTreeNodeAllocator() : mPtr(4096) {}

        SceneBVHTreeNode *Allocate()
        {
            if (mPtr == 4096)
            {
                mNodesList.push_back(new SceneBVHTreeNode[4096]);
                mPtr = 0;
            }
            return &(mNodesList.back()[mPtr++]);
        }

        ~SceneBVHTreeNodeAllocator()
        {
            for (auto *nodes : mNodesList)
            {
                delete[] nodes;
            }
            mNodesList.clear();
        }
    };

    class SceneBVH : public Shape
    {
    public:
        void Build(std::vector<ShapeInfo> &&shapeInfos);
        std::optional<HitInfo> Intersect(const Ray &ray, float t_min, float t_max) const override;
        Bounds GetBounds() const override { return mNodes[0].__bounds__; }

    private:
        void RecursiveSplitBySAHB(SceneBVHTreeNode *node, SceneBVHState &state);
        size_t RecursiveFlatten(SceneBVHTreeNode *node);

    private:
        std::vector<SceneBVHNode> mNodes;
        std::vector<ShapeInfo> mOrderedShapeInfos;
        std::vector<ShapeInfo> mInfinityShapeInfos;
        SceneBVHTreeNodeAllocator mNodeAllocator{};
    };
}
