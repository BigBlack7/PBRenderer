#pragma once

#include "spinLock.hpp"
#include <vector>
#include <thread>
#include <queue>
#include <functional>

namespace pt
{
    class Task
    {
    public:
        virtual ~Task() = default;
        virtual void Run() = 0;
    };

    class ThreadPool
    {
    private:
        std::vector<std::thread> mThreads;
        std::queue<Task *> mTasks;
        SpinLock mLock{};
        std::atomic<int> mAlive;
        std::atomic<int> mPendingTaskCount;

    public:
        static void WorkerThread(ThreadPool *master);

        ThreadPool(size_t thread_count = 0);
        ~ThreadPool();

        void ParallelFor(size_t width, size_t height, const std::function<void(size_t, size_t)> &lambda, bool isComplex = true);
        void Wait() const;

        void AddTask(Task *task);
        Task *GetTask();
    };

    extern ThreadPool threadPool;
}
