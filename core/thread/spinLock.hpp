#pragma once

#include <atomic>
#include <thread>

namespace pbrt
{
    class SpinLock
    {
    private:
        std::atomic_flag mFlag{};

    public:
        void Acquire()
        {
            while (mFlag.test_and_set(std::memory_order_acquire))
            {
                std::this_thread::yield();
            }
        }

        void Release()
        {
            mFlag.clear(std::memory_order_release);
        }
    };

    class Guard
    {
    private:
        SpinLock &mLock;

    public:
        Guard(SpinLock &lock) : mLock(lock)
        {
            mLock.Acquire();
        }
        ~Guard()
        {
            mLock.Release();
        }
    };
}