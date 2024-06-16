#include "Debug.h"

#include <chrono>
#include <string>
#include <fstream>
#include <stdio.h>
#include <time.h>

static std::string TimeStamp()
{
    time_t rawtime;
    struct tm timeinfo;
    char buffer[80];

    time(&rawtime);
    localtime_s(&timeinfo, &rawtime);

    strftime(buffer, 80, "%F %H:%M:%S", &timeinfo);

    return buffer;
}

namespace debug
{
    void WriteLog(const char *type, const std::string& text)
    {
        static std::ofstream log("debug.log", std::ios_base::out | std::ios_base::app);
        log << TimeStamp() << ": " << type << "\t| " << text << std::endl;
    }

    void info(const std::string& text)
    {
        WriteLog("INFO", text);
    }

    void warn(const std::string& text)
    {
        WriteLog("WARN", text);
    }

    void err(const std::string & text)
    {
        WriteLog("ERR", text);
    }
}