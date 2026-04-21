// shared/src/Utils.cpp
#include <stdlib.h>
#include <math.h>
#include "Utils.h"
#include <algorithm>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <random>

namespace Atlas::Utils {

std::string ToLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), ::tolower); return s;
}
std::string ToUpper(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), ::toupper); return s;
}
std::string Trim(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\r\n"), b = s.find_last_not_of(" \t\r\n");
    return a == std::string::npos ? "" : s.substr(a, b - a + 1);
}
std::vector<std::string> Split(const std::string& s, char d) {
    std::vector<std::string> o; std::stringstream ss(s); std::string t;
    while (std::getline(ss, t, d)) if (!t.empty()) o.push_back(t);
    return o;
}
uint64_t TimeMs() {
    return (uint64_t)std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}
uint64_t TimeUs() {
    return (uint64_t)std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}
std::string FormatTime(uint64_t sec) {
    std::ostringstream ss;
    ss << std::setfill('0') << std::setw(2) << sec/3600 << ":"
       << std::setw(2) << (sec%3600)/60 << ":" << std::setw(2) << sec%60;
    return ss.str();
}
std::string RandomString(size_t len) {
    static const char c[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    static std::mt19937 rng(std::random_device{}());
    static std::uniform_int_distribution<size_t> d(0, sizeof(c)-2);
    std::string o(len, ' '); for (auto& x : o) x = c[d(rng)]; return o;
}
uint32_t HashString(const std::string& s) {
    uint32_t h = 2166136261u;
    for (unsigned char c : s) { h ^= c; h *= 16777619u; }
    return h;
}

} // namespace Atlas::Utils
