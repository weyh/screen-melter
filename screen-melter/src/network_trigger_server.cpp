#include "network_trigger_server.h"
#include "framework.h"
#include "debug.h"

#include <array>
#include <chrono>
#include <format>
#include <utility>

namespace {
    constexpr size_t maxRequestSize = 512;

    class ScopedSocket {
    public:
        explicit ScopedSocket(SOCKET socket) : socket_(socket) {}
        ~ScopedSocket() { if (socket_ != INVALID_SOCKET) closesocket(socket_); }

        ScopedSocket(const ScopedSocket&) = delete;
        ScopedSocket& operator=(const ScopedSocket&) = delete;

        SOCKET Get() const { return socket_; }
    private:
        SOCKET socket_;
    };

    std::string SocketError(const char* operation, int error)
    {
        if (error == WSAEADDRINUSE)
            return std::format("{} failed: the listening port is already in use (Winsock error {})", operation, error);

        if (error == WSAEACCES)
            return std::format("{} failed: access was denied for the requested port (Winsock error {})", operation, error);

        return std::format("{} failed (Winsock error {})", operation, error);
    }
}

NetworkTriggerServer::NetworkTriggerServer(int port, int timeoutMs, std::string key, std::function<void()> onAuthenticated)
    : port_(port), timeoutMs_(timeoutMs), key_(std::move(key)), 
onAuthenticated_(std::move(onAuthenticated))
{}

NetworkTriggerServer::~NetworkTriggerServer()
{
    Stop();
    if (winsockStarted_)
        WSACleanup();
}

bool NetworkTriggerServer::Start(std::string& error)
{
    WSADATA data{};
    const int startupResult = WSAStartup(MAKEWORD(2, 2), &data);
    if (startupResult != 0) {
        error = SocketError("WSAStartup", startupResult);
        return false;
    }
    winsockStarted_ = true;

    listener_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener_ == INVALID_SOCKET) {
        error = SocketError("socket", WSAGetLastError());
        return false;
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(static_cast<u_short>(port_));

    if (bind(listener_, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == SOCKET_ERROR) {
        error = SocketError("bind", WSAGetLastError());
        closesocket(listener_);
        listener_ = INVALID_SOCKET;
        return false;
    }

    if (listen(listener_, 4) == SOCKET_ERROR) {
        error = SocketError("listen", WSAGetLastError());
        closesocket(listener_);
        listener_ = INVALID_SOCKET;
        return false;
    }

    stopping_ = false;
    worker_ = std::thread(&NetworkTriggerServer::Run, this);
    return true;
}

void NetworkTriggerServer::Stop()
{
    stopping_ = true;
    {
        std::lock_guard<std::mutex> lock(socketMutex_);
        if (listener_ != INVALID_SOCKET)
            shutdown(listener_, SD_BOTH);

        if (activeClient_ != INVALID_SOCKET)
            shutdown(activeClient_, SD_BOTH);
    }

    if (worker_.joinable())
        worker_.join();

    if (listener_ != INVALID_SOCKET) {
        closesocket(listener_);
        listener_ = INVALID_SOCKET;
    }
}

void NetworkTriggerServer::Run()
{
    while (!stopping_) {
        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(listener_, &readSet);

        timeval pollTime{ 0, 200000 };

        const int ready = select(0, &readSet, nullptr, nullptr, &pollTime);
        if (stopping_)
            break;

        if (ready == SOCKET_ERROR) {
            DEBUG_ERR("Network listener select failed (Winsock error {})", WSAGetLastError());
            break;
        }

        if (ready == 0)
            continue;

        SOCKET accepted = accept(listener_, nullptr, nullptr);
        if (accepted == INVALID_SOCKET) {
            if (!stopping_)
                DEBUG_WARN("Network listener accept failed (Winsock error {})", WSAGetLastError());
            continue;
        }

        ScopedSocket client(accepted);
        {
            std::lock_guard<std::mutex> lock(socketMutex_);
            activeClient_ = accepted;
        }

        const bool authenticated = HandleClient(accepted);

        {
            std::lock_guard<std::mutex> lock(socketMutex_);
            activeClient_ = INVALID_SOCKET;
        }

        if (authenticated) {
            if (onAuthenticated_)
                onAuthenticated_();
            break;
        }
    }
}

bool NetworkTriggerServer::HandleClient(SOCKET client)
{
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs_);
    std::string request;
    request.reserve(maxRequestSize);
    std::array<char, 128> buffer{};

    while (!stopping_) {
        const auto now = std::chrono::steady_clock::now();
        if (now >= deadline)
            return false;

        const auto remaining = std::chrono::duration_cast<std::chrono::microseconds>(deadline - now);

        timeval waitTime{};
        waitTime.tv_sec = static_cast<long>(remaining.count() / 1000000);
        waitTime.tv_usec = static_cast<long>(remaining.count() % 1000000);

        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(client, &readSet);

        const int ready = select(0, &readSet, nullptr, nullptr, &waitTime);
        if (ready <= 0)
            return false;

        const int received = recv(client, buffer.data(), static_cast<int>(buffer.size()), 0);
        if (received <= 0)
            return false;

        for (int i = 0; i < received; i++) {
            const char character = buffer[static_cast<size_t>(i)];
            if (character == '\n') {
                if (i != received - 1)
                    return false;

                constexpr char prefix[] = "SM1:";
                if (request.size() < sizeof(prefix) - 1 || request.compare(0, sizeof(prefix) - 1, prefix) != 0)
                    return false;

                const std::string receivedKey = request.substr(sizeof(prefix) - 1);

                return !receivedKey.empty() && receivedKey == key_;
            }

            if (character == '\r' || request.size() >= maxRequestSize - 1)
                return false;

            request.push_back(character);
        }
    }
    return false;
}
