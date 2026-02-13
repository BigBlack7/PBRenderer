#include "threadPool.hpp"

namespace pbrt
{
    ThreadPool MasterThreadPool{};
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
                std::this_thread::yield(); // 让出操作权给os, 让os选择就绪线程执行
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

    struct ParallelTask : public Task // 任务块
    {
    private:
        size_t /* 块起点 */ __x__, __y__, /* 块大小 */ __chunkWidth__, __chunkHeight__;
        std::function<void(size_t, size_t)> __lambda__;

    public:
        ParallelTask(size_t x, size_t y, size_t chunk_width, size_t chunk_height, const std::function<void(size_t, size_t)> &lambda) : __x__(x), __y__(y), __chunkWidth__(chunk_width), __chunkHeight__(chunk_height), __lambda__(lambda) {}

        void Run() override
        {
            // 遍历块内元素
            for (size_t j = 0; j < __chunkHeight__; j++)
            {
                for (size_t i = 0; i < __chunkWidth__; i++)
                {
                    __lambda__(__x__ + i, __y__ + j);
                }
            }
        }
    };

    /*
        Example1(width=2000, chunk_width=480):
        (0,0)        (480,0)      (960,0)      (1440,0)
        ┌────────────┬────────────┬────────────┬────────────┐
        │  Task1     │  Task2     │  Task3     │  Task4     │
        │  480x270   │  480x270   │  480x270   │  480x270   │
        ├────────────┼────────────┼────────────┼────────────┤
        │  Task5     │  Task6     │  Task7     │  Task8     │
        │  480x270   │  480x270   │  480x270   │  480x270   │
        ├────────────┼────────────┼────────────┼────────────┤
        │  Task9     │  Task10    │  Task11    │  Task12    │
        │  480x270   │  480x270   │  480x270   │  480x270   │
        ├────────────┼────────────┼────────────┼────────────┤
        │  Task13    │  Task14    │  Task15    │  Task16    │
        │  480x270   │  480x270   │  480x270   │  480x270   │
        └────────────┴────────────┴────────────┴────────────┘
        (0,810)      (480,810)    (960,810)    (1440,810)

        Example2:
        正常块：480×270
        ┌──────────────────────────────────────────────────────────┐
        │                                                          │
        │ ┌────┬────┬────┬────┐                                    │
        │ │    │    │    │    │                                    │
        │ │ 480│ 480│ 480│ 80 │ ← 边界自适应: 最后一块宽度=80        │
        │ │ ×  │ ×  │ ×  │ ×  │                                    │
        │ │270 │270 │270 │270 │                                    │
        │ │    │    │    │    │                                    │
        │ └────┴────┴────┴────┘                                    │
        │  0   480  960  1440 2000 ← x坐标                         │
        │                                                          │
        └──────────────────────────────────────────────────────────┘
    */
    void ThreadPool::ParallelFor(size_t width, size_t height, const std::function<void(size_t, size_t)> &lambda, bool is_complex)
    {
        Guard guard(mLock);

        // 将大量任务分块, 减少new Task的次数
        float chunk_width_float = static_cast<float>(width) / std::sqrt(mThreads.size());
        float chunk_height_float = static_cast<float>(height) / std::sqrt(mThreads.size());
        if (is_complex)
        {
            // 复杂任务时, 进一步减小块大小
            chunk_width_float /= std::sqrt(16);
            chunk_height_float /= std::sqrt(16);
        }
        size_t chunk_width = std::ceil(chunk_width_float);
        size_t chunk_height = std::ceil(chunk_height_float);

        // 添加所有任务块, 列优先遍历提高cache命中率
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