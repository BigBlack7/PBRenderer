#pragma once
#include <memory>
#include <spdlog/spdlog.h>

namespace core
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
#define PBRT_TRACE(...) ::core::Logger::GetCoreLogger()->trace(__VA_ARGS__)
#define PBRT_DEBUG(...) ::core::Logger::GetCoreLogger()->debug(__VA_ARGS__)
#define PBRT_INFO(...) ::core::Logger::GetCoreLogger()->info(__VA_ARGS__)
#define PBRT_WARN(...) ::core::Logger::GetCoreLogger()->warn(__VA_ARGS__)
#define PBRT_ERROR(...) ::core::Logger::GetCoreLogger()->error(__VA_ARGS__)
#define PBRT_CRITICAL(...) ::core::Logger::GetCoreLogger()->critical(__VA_ARGS__)
