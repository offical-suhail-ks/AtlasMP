#pragma once
#ifndef _USE_MATH_DEFINES
#  define _USE_MATH_DEFINES
#endif
#ifdef _MSC_VER
#  include <corecrt_math.h>
#endif
#include <cmath>
#include <cstdlib>
// server/src/core/Logger.h

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <filesystem>
#include <string>

namespace Atlas {

class Logger {
public:
    static void Init(const std::string& logFile = "logs/server.log") {
        try {
            std::filesystem::create_directories("logs");
            auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
            consoleSink->set_pattern("[%H:%M:%S] [%^%l%$] %v");
            auto fileSink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
                logFile, 1024 * 1024 * 10, 5);
            fileSink->set_pattern("[%Y-%m-%d %H:%M:%S] [%l] %v");
            auto logger = std::make_shared<spdlog::logger>("atlas",
                spdlog::sinks_init_list{ consoleSink, fileSink });
            logger->set_level(spdlog::level::debug);
            spdlog::set_default_logger(logger);
        } catch (...) {
            spdlog::set_pattern("[%H:%M:%S] [%^%l%$] %v");
        }
    }

    template<typename... Args>
    static void Info(spdlog::format_string_t<Args...> fmt, Args&&... args) {
        spdlog::info(fmt, std::forward<Args>(args)...);
    }
    template<typename... Args>
    static void Warn(spdlog::format_string_t<Args...> fmt, Args&&... args) {
        spdlog::warn(fmt, std::forward<Args>(args)...);
    }
    template<typename... Args>
    static void Error(spdlog::format_string_t<Args...> fmt, Args&&... args) {
        spdlog::error(fmt, std::forward<Args>(args)...);
    }
    template<typename... Args>
    static void Debug(spdlog::format_string_t<Args...> fmt, Args&&... args) {
        spdlog::debug(fmt, std::forward<Args>(args)...);
    }
};

} // namespace Atlas
