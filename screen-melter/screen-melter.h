#pragma once

#include "resource.h"
#include <iostream>
#include <fstream>

void WriteLog(std::string s)
{
    std::ofstream myfile;
    myfile.open ("debug.log");
    myfile << s;
    myfile.close();
}