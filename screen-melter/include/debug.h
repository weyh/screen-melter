#pragma once

#include <string>
#include <format>

#ifdef _DEBUG

namespace debug
{
    void info(const std::string& text);
    void warn(const std::string& text);
    void err(const std::string& text);
}

#define DEBUG_INFO(fmt, ...) \
    debug::info(std::format(fmt, __VA_ARGS__))

#define DEBUG_WARN(fmt, ...) \
    debug::warn(std::format(fmt, __VA_ARGS__))

#define DEBUG_ERR(fmt, ...) \
    debug::err(std::format(fmt, __VA_ARGS__))
#else

#define DEBUG_INFO(fmt, ...) \
    do {} while(0)

#define DEBUG_WARN DEBUG_INFO
#define DEBUG_ERR DEBUG_INFO

#endif
