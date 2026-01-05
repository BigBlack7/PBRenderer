#pragma once
#include <atomic>
#include <thread>

namespace pbrt
{
    /*
        自旋锁, 忙等待锁, 线程在获取锁失败时不会休眠, 而是持续检查锁状态
        短时间锁定, 避免线程上下文切换开销
    */
    class SpinLock
    {
    private:
        std::atomic_flag mFlag{}; // 原子标志, 保证线程安全(只包含设置与清除两种状态), 默认初始化为清除

    public:
        void Acquire()
        {
            /*
                test_and_set(): 原子地设置标志并返回之前的值
                memory_order_acquire: 获取内存序, 确保之前的写操作对当前线程可见
                如果返回true(即之前已被设置), 说明锁被占用, 继续循环
            */
            while (mFlag.test_and_set(std::memory_order_acquire))
            {
                std::this_thread::yield(); // 获取失败时让出CPU, 避免过度占用
            }
        }

        void Release()
        {
            // memory_order_release: 释放内存序, 确保当前线程的修改对其他线程可见
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