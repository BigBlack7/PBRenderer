#pragma once
#include <memory>
#include <spdlog/spdlog.h>

namespace pbrt
{
    class Logger
    {
    private:
        static std::shared_ptr<spdlog::logger> mLogger;

    public:
        static void Init();
        inline static std::shared_ptr<spdlog::logger> &GetCoreLogger() { return mLogger; }
    };
}

// 日志宏
#define PBRT_TRACE(...) ::pbrt::Logger::GetCoreLogger()->trace(__VA_ARGS__)
#define PBRT_DEBUG(...) ::pbrt::Logger::GetCoreLogger()->debug(__VA_ARGS__)
#define PBRT_INFO(...) ::pbrt::Logger::GetCoreLogger()->info(__VA_ARGS__)
#define PBRT_WARN(...) ::pbrt::Logger::GetCoreLogger()->warn(__VA_ARGS__)
#define PBRT_ERROR(...) ::pbrt::Logger::GetCoreLogger()->error(__VA_ARGS__)
#define PBRT_CRITICAL(...) ::pbrt::Logger::GetCoreLogger()->critical(__VA_ARGS__)
