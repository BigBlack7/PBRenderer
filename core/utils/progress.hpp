#pragma once

#include "thread/threadPool.hpp"

namespace pt
{
    class Progress
    {
    private:
        size_t mTotal, mCurrent;
        size_t mPercent, mLastPercent, mStep;
        SpinLock mLock;

    public:
        Progress(size_t total, size_t step = 2);

        void Update(size_t count);
    };
}