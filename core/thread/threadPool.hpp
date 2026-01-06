#pragma once
#include "spinLock.hpp"
#include <vector>
#include <thread>
#include <queue>
#include <functional>

namespace pbrt
{
    class Task
    {
    public:
        virtual ~Task() = default; // 虚析构函数, 确保派生类正确析构
        virtual void Run() = 0;
    };

    /*
        主线程 → 添加任务 → 工作线程获取 → 执行任务 → 完成通知
    */
    class ThreadPool
    {
    private:
        std::vector<std::thread> mThreads;  // 线程池, 存储所有工作线程
        std::queue<Task *> mTasks;          // 任务队列
        SpinLock mLock{};                   // 线程同步
        std::atomic<int> mAlive;            // 线程池存活标志
        std::atomic<int> mPendingTaskCount; // 待处理任务计数

    public:
        static void WorkerThread(ThreadPool *master);

        ThreadPool(size_t thread_count = 0);
        ~ThreadPool();

        void ParallelFor(size_t width, size_t height, const std::function<void(size_t, size_t)> &lambda, bool is_complex = true);
        void Wait() const;

        void AddTask(Task *task);
        Task *GetTask();
    };

    extern ThreadPool MasterThreadPool;
}