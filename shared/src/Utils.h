#pragma once
// shared/src/Utils.h
#include <string>
#include <vector>
#include <cstdint>

namespace Atlas::Utils {

std::string              ToLower(std::string s);
std::string              ToUpper(std::string s);
std::string              Trim(const std::string& s);
std::vector<std::string> Split(const std::string& s, char delim);
uint64_t                 TimeMs();
uint64_t                 TimeUs();
std::string              FormatTime(uint64_t seconds);
std::string              RandomString(size_t len);
uint32_t                 HashString(const std::string& s);

} // namespace Atlas::Utils
