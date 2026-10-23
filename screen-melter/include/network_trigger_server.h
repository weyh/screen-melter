#pragma once

#include "targetver.h"

#include <atomic>
#include <functional>
#include <mutex>
#include <string>
#include <thread>

#include <winsock2.h>

class NetworkTriggerServer {
public:
    NetworkTriggerServer(int port, int timeoutMs, std::string key, std::function<void()> onAuthenticated);
    ~NetworkTriggerServer();

    NetworkTriggerServer(const NetworkTriggerServer&) = delete;
    NetworkTriggerServer& operator=(const NetworkTriggerServer&) = delete;

    bool Start(std::string& error);
    void Stop();

private:
    void Run();
    bool HandleClient(SOCKET client);

    int port_;
    int timeoutMs_;
    std::string key_;
    std::function<void()> onAuthenticated_;
    SOCKET listener_ = INVALID_SOCKET;
    SOCKET activeClient_ = INVALID_SOCKET;
    std::mutex socketMutex_;
    std::atomic<bool> stopping_ = false;
    bool winsockStarted_ = false;
    std::thread worker_;
};
