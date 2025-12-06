#include "logger.hpp"
#include <windows.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace pt
{
    std::shared_ptr<spdlog::logger> Logger::mLogger;

    void Logger::Init()
    {
        // 设置控制台编码为UTF-8, 避免中文日志乱码
        SetConsoleOutputCP(CP_UTF8);
        SetConsoleCP(CP_UTF8);

        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_pattern("[%T] [%^%l%$] %v");

        mLogger = std::make_shared<spdlog::logger>("PBRT", console_sink);
        mLogger->set_level(spdlog::level::trace); // 输出所有级别
        mLogger->flush_on(spdlog::level::warn);   // WARN 及以上即时刷新
    }
}
