#pragma once
#include <glm/glm.hpp>
#include <memory>

namespace pbrt
{
    // 低差异序列采样器(伪随机和准随机)
    class Sampler
    {
    public:
        virtual ~Sampler() = default;

        // 获取[0, 1)区间的单个随机样本
        virtual float Get1D() const = 0;

        // 获取[0, 1)²区间的二维随机样本
        virtual glm::vec2 Get2D() const = 0;

        // 为新的像素和样本索初始化采样器
        // pixel: 像素坐标
        // sample_index: 该像素的样本编号(0, 1, 2, ...)
        virtual void StartPixelSample(const glm::ivec2 &pixel, int sample_index) = 0;

        // 克隆采样器以供其他线程使用
        virtual std::unique_ptr<Sampler> Clone() const = 0;

        // 获取当前样本索引
        virtual int GetSampleIndex() const = 0;
    };
}