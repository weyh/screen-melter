#pragma once

#include <chrono>
#include <thread>
#include <string>

inline std::string btos(const bool &b)
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

struct Vector4Int
{
    int w, x, y, z;
};