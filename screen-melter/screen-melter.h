#pragma once

#include "resource.h"
#include <fstream>

void WriteLog(const char* text)
{
#ifdef _DEBUG
    std::ofstream log("debug.log", std::ios_base::out | std::ios_base::app);
    log << text << std::endl;
#endif
}