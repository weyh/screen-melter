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
    void WriteLog(const std::string& text)
    {
#ifdef _DEBUG
        static std::ofstream log("debug.log", std::ios_base::out | std::ios_base::app);

        log << TimeStamp() << ": " << text << std::endl;
#endif
    }
}