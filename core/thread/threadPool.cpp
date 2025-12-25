#include "threadPool.hpp"

namespace pbrt
{
    ThreadPool threadPool{};
    void ThreadPool::WorkerThread(ThreadPool *master)
    {
        while (master->mAlive)
        {
            if (master->mTasks.empty())
            {
                // 解决工作线程空转问题, 防止与BVH构建线程竞争
                std::this_thread::sleep_for(std::chrono::milliseconds(2));
                continue;
            }
            Task *task = master->GetTask();
            if (task != nullptr)
            {
                task->Run();
                delete task;
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
        size_t __x__, __y__, __chunkWidth__, __chunkHeight__;
        std::function<void(size_t, size_t)> __lambda__;

    public:
        ParallelTask(size_t x, size_t y, size_t chunk_width, size_t chunk_height, const std::function<void(size_t, size_t)> &lambda) : __x__(x), __y__(y), __chunkWidth__(chunk_width), __chunkHeight__(chunk_height), __lambda__(lambda) {}

        void Run() override
        {
            for (size_t i = 0; i < __chunkWidth__; i++)
            {
                for (size_t j = 0; j < __chunkHeight__; j++)
                {
                    __lambda__(__x__ + i, __y__ + j);
                }
            }
        }
    };

    void ThreadPool::ParallelFor(size_t width, size_t height, const std::function<void(size_t, size_t)> &lambda, bool isComplex)
    {
        Guard guard(mLock);

        // 将大量任务分块, 减少new Task的次数
        float chunk_width_float = static_cast<float>(width) / std::sqrt(mThreads.size());
        float chunk_height_float = static_cast<float>(height) / std::sqrt(mThreads.size());
        if (isComplex)
        {
            chunk_width_float /= std::sqrt(16);
            chunk_height_float /= std::sqrt(16);
        }
        size_t chunk_width = std::ceil(chunk_width_float);
        size_t chunk_height = std::ceil(chunk_height_float);

        for (size_t x = 0; x < width; x += chunk_width)
        {
            size_t W = ((x + chunk_width) > width) ? (width - x) : chunk_width;
            for (size_t y = 0; y < height; y += chunk_height)
            {
                mPendingTaskCount++;
                size_t H = ((y + chunk_height) > height) ? (height - y) : chunk_height;
                mTasks.push(new ParallelTask(x, y, W, H, lambda));
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