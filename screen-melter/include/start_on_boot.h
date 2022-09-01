#pragma once

#include <string>
#include <winerror.h>

namespace StartOnBoot {
    bool StartUpLinkExits();

    void CleanUpStartOnBoot();

    HRESULT SetupStartOnBoot(const std::string& args);
}