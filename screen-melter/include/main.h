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
    bool killExpolerOnStart;
    bool topmost;
    bool enableNetwork;
    int port;
    int networkTimeout;

    std::string startupArgs;
    std::string networkKey;

    AppArgs() : time(0), exitTime(-1), disableInput(false), disableKeyboard(false), disableMouse(false),
        killExpolerOnStart(false), topmost(false), startupArgs(""),
        enableNetwork(false), port(47621), networkTimeout(5000), networkKey("")
    { }
};
