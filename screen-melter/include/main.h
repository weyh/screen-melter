#pragma once

#include <chrono>
#include <thread>
#include <string>

template<typename T>
struct Vector4
{
    T w, x, y, z;
};

class AppArgs {
public:
    int time;
    int exitTime;
    bool disableInput;
    bool disableKeyboard;
    bool disableMouse;

    std::string startupArgs;

    bool killExpolerOnStart;

    AppArgs() : time(0), exitTime(-1), disableInput(false), disableKeyboard(false), disableMouse(false),
        startupArgs(""), killExpolerOnStart(false)
    { }
};