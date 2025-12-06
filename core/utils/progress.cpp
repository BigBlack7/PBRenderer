#pragma once
#include "progress.hpp"
#include "logger.hpp"

namespace pt
{
    Progress::Progress(size_t total, size_t step) : mTotal(total), mCurrent(0), mPercent(0), mLastPercent(0), mStep(step)
    {
        PBRT_INFO("Render - 0%");
    }

    void Progress::Update(size_t count)
    {
        Guard guard(mLock);

        mCurrent += count;
        mPercent = static_cast<float>(mCurrent) / static_cast<float>(mTotal) * 100.f;
        if ((mPercent - mLastPercent >= mStep) || (mPercent == 100.f))
        {
            mLastPercent = mPercent;
            PBRT_INFO("Render - {}%", mPercent);
        }
    }
}