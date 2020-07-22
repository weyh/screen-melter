#pragma once

#include "resource.h"
#include <chrono>
#include <thread>
#include <string>

#ifdef _DEBUG
#include <fstream>
#include <stdio.h>
#include <time.h>
#endif

std::string BoolToString(bool b)
{
    return b ? "true" : "false";
}

void WaitAndSet(int sleepTime, bool* boolToSet)
{
    if(sleepTime > -1)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(sleepTime));
        *boolToSet = true;
    }
}

class Debug
{
#ifdef _DEBUG
private:
    static std::string TimeStamp()
    {
        time_t rawtime;
        struct tm timeinfo;
        char buffer[80];

        time (&rawtime);
        localtime_s(&timeinfo , &rawtime);

        strftime(buffer, 80, "%F %H:%M:%S", &timeinfo);
        puts(buffer);

        return buffer;
    }

    static void _WriteLog(const char* text)
    {
        std::ofstream log("debug.log", std::ios_base::out | std::ios_base::app);
        log << TimeStamp() << ": " << text << std::endl;
    }
#endif

public:
    static void WriteLog(const char* text)
    {
    #ifdef _DEBUG
        _WriteLog(text);
    #endif
    }

    static void WriteLog(std::string text)
    {
    #ifdef _DEBUG
        _WriteLog(text.c_str());
    #endif
    }
};