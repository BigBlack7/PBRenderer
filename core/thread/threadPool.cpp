#include "threadPool.hpp"

namespace pt
{
    ThreadPool threadPool{};
    void ThreadPool::WorkerThread(ThreadPool *master)
    {
        while (master->mAlive)
        {
            Task *task = master->GetTask();
            if (task != nullptr)
            {
                task->Run();
                master->mPendingTaskCount--;
            }
            else
            {
                std::this_thread::yield(); // 让出操作权给os, 让os选择下一个线程执行
            }
        }
    }

    ThreadPool::ThreadPool(size_t thread_count)
    {
        mAlive = 1;
        mPendingTaskCount = 0;
        if (thread_count == 0)
        {
            // 赋值线程数为CPU线程数
            thread_count = std::thread::hardware_concurrency();
        }

        for (size_t i = 0; i < thread_count; i++)
        {
            // 添加工作线程
            mThreads.push_back(std::thread(ThreadPool::WorkerThread, this));
        }
    }

    ThreadPool::~ThreadPool()
    {
        Wait();
        mAlive = 0;
        // 等待所有线程执行完毕后清空线程池
        for (auto &thread : mThreads)
        {
            thread.join();
        }
        mThreads.clear();
    }

    struct ParallelTask : public Task
    {
    private:
        size_t __x__, __y__;
        std::function<void(size_t, size_t)> __lambda__;

    public:
        ParallelTask(size_t x, size_t y, const std::function<void(size_t, size_t)> &lambda) : __x__(x), __y__(y), __lambda__(lambda) {}

        void Run() override
        {
            __lambda__(__x__, __y__);
        }
    };

    void ThreadPool::ParallelFor(size_t width, size_t height, const std::function<void(size_t, size_t)> &lambda)
    {
        Guard guard(mLock);
        for (size_t x = 0; x < width; x++)
        {
            for (size_t y = 0; y < height; y++)
            {
                mPendingTaskCount++;
                mTasks.push(new ParallelTask(x, y, lambda));
            }
        }
    }

    void ThreadPool::Wait() const
    {
        while (mPendingTaskCount > 0)
        {
            std::this_thread::yield();
        }
    }

    void ThreadPool::AddTask(Task *task)
    {
        Guard guard(mLock);
        mPendingTaskCount++;
        mTasks.push(task);
    }

    Task *ThreadPool::GetTask()
    {
        Guard guard(mLock);
        if (mTasks.empty())
        {
            return nullptr;
        }
        Task *task = mTasks.front();
        mTasks.pop();
        return task;
    }
}