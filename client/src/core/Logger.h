#pragma once
// client/src/core/Logger.h
// Client-side file logger (no spdlog dependency in client to keep DLL lean)
#include <string>
#include <fstream>
#include <mutex>
#include <cstdio>
#include <Windows.h>
#ifndef _USE_MATH_DEFINES
#  define _USE_MATH_DEFINES
#endif
#ifdef _MSC_VER
#  include <corecrt_math.h>
#endif
#include <cmath>
#include <cstdlib>

namespace Atlas {

class Logger {
public:
    static void Init(const std::string& filename = "AtlasMP-client.log") {
        std::lock_guard<std::mutex> lock(s_mutex);
        s_file.open(filename, std::ios::app);
    }

    static void Info(const std::string& msg)  { Write("[INFO ] " + msg); }
    static void Warn(const std::string& msg)  { Write("[WARN ] " + msg); }
    static void Error(const std::string& msg) { Write("[ERROR] " + msg); }
    static void Debug(const std::string& msg) { Write("[DEBUG] " + msg); }

    template<typename... Args>
    static void Info(const char* fmt, Args... args)  { Write("[INFO ] " + Format(fmt, args...)); }
    template<typename... Args>
    static void Warn(const char* fmt, Args... args)  { Write("[WARN ] " + Format(fmt, args...)); }
    template<typename... Args>
    static void Error(const char* fmt, Args... args) { Write("[ERROR] " + Format(fmt, args...)); }
    template<typename... Args>
    static void Debug(const char* fmt, Args... args) { Write("[DEBUG] " + Format(fmt, args...)); }

private:
    static void Write(const std::string& line) {
        std::lock_guard<std::mutex> lock(s_mutex);
        if (s_file.is_open()) {
            s_file << line << "\n";
            s_file.flush();
        }
        // Also output to VS debugger
        OutputDebugStringA((line + "\n").c_str());
    }

    template<typename... Args>
    static std::string Format(const char* fmt, Args... args) {
        char buf[1024];
        snprintf(buf, sizeof(buf), fmt, args...);
        return buf;
    }

    static inline std::ofstream s_file;
    static inline std::mutex    s_mutex;
};

} // namespace Atlas
