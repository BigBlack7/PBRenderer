#pragma once
#include <memory>
#include <spdlog/spdlog.h>

namespace pt
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
#define PBRT_TRACE(...) ::pt::Logger::GetCoreLogger()->trace(__VA_ARGS__)
#define PBRT_DEBUG(...) ::pt::Logger::GetCoreLogger()->debug(__VA_ARGS__)
#define PBRT_INFO(...) ::pt::Logger::GetCoreLogger()->info(__VA_ARGS__)
#define PBRT_WARN(...) ::pt::Logger::GetCoreLogger()->warn(__VA_ARGS__)
#define PBRT_ERROR(...) ::pt::Logger::GetCoreLogger()->error(__VA_ARGS__)
#define PBRT_CRITICAL(...) ::pt::Logger::GetCoreLogger()->critical(__VA_ARGS__)
