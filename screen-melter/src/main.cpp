// Modified by weyh
// Inspired by Napalm/MetalHead (http://www.rohitab.com/discuss/topic/23191-screen-melter/?p=190669)

#include "network_trigger_server.h"
#include "framework.h"
#include "main.h"
#include "start_on_boot.h"
#include "debug.h"

#include "CLI11.hpp"

#include <iostream>
#include <chrono>
#include <thread>
#include <string>
#include <utility>
#include <tlhelp32.h>

constexpr int meltWidth = 150;
constexpr int meltHeight = 15;
constexpr int interval = 1;
constexpr UINT WM_NETWORK_TRIGGER = WM_APP + 1;
Vector4<int> screen;
HHOOK keyboardHhook;

LRESULT CALLBACK KeyboardHook(int code, WPARAM wParam, LPARAM lParam)
{
    KBDLLHOOKSTRUCT* kbd = (KBDLLHOOKSTRUCT*)lParam;

    // ignore injected events
    if(code < 0 || (kbd->flags & 0x10))
        return CallNextHookEx(keyboardHhook, code, wParam, lParam);

    if(WM_KEYDOWN == wParam)
        DEBUG_INFO("WM_KEYDOWN");

    return 1; // by default swallow keys
}

LRESULT WINAPI MelterProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    switch(Msg)
    {
        case WM_CREATE:
            {
                HDC hdcDesktop = GetDC(HWND_DESKTOP);
                HDC hdcWindow = GetDC(hWnd);

                BitBlt(hdcWindow, screen.w, screen.x, screen.y, screen.z, hdcDesktop, 0, 0, SRCCOPY);

                ReleaseDC(hWnd, hdcWindow);
                ReleaseDC(HWND_DESKTOP, hdcDesktop);

                SetTimer(hWnd, 0, interval, NULL);

                ShowWindow(hWnd, SW_SHOW);
            }
            return 0;
        case WM_ERASEBKGND:
            return 0;
        case WM_PAINT:
            ValidateRect(hWnd, NULL);
            return 0;
        case WM_TIMER:
            {
                HDC hdcWindow = GetDC(hWnd);

                int x = (rand() % screen.y) - (meltWidth / 2);
                int y = (rand() % meltHeight);
                int width = (rand() % meltWidth);

                BitBlt(hdcWindow, x, y, width, screen.z, hdcWindow, x, 0, SRCCOPY);
                ReleaseDC(hWnd, hdcWindow);
            }
            return 0;
        case WM_CLOSE:
#ifndef _DEBUG
            return 0; // disable closing
#endif
        case WM_DESTROY:
            KillTimer(hWnd, 0);
            PostQuitMessage(0);
            return 0;
    }

    return DefWindowProc(hWnd, Msg, wParam, lParam);
}

static inline std::string btos(bool b)
{
    return b ? "true" : "false";
}

/// <summary>
/// Parses the command line arguments.
/// </summary>
/// <param name="appArgs">Data will be stored here.</param>
/// <returns>0 on success</returns>
static int ParseArgs(AppArgs& appArgs)
{
    CLI::App app("Creates melting like effect on users screen.", "Screen Melter");

    app.add_option("-t,--time", appArgs.time, "Sleep time before visual effect (ms)");
    app.add_option("-e,--exit_time", appArgs.exitTime, "Sleep time before visual effect (ms)");

    app.add_option("-B,--start_on_boot", appArgs.startupArgs, "Run screen melter on next boot with given args");

    app.add_flag("-I,--disable_input", appArgs.disableInput, "Disable user input");
    app.add_flag("-K,--disable_keyboard", appArgs.disableKeyboard, "Disable user keyboard");
    app.add_flag("-M,--disable_mouse", appArgs.disableMouse, "Disable user mouse");
    app.add_flag("-k,--kill_explorer", appArgs.killExpolerOnStart, "Kill explorer process and restart when program closes");
    app.add_flag("-T,--topmost", appArgs.topmost, "Runs window always on top");

    app.add_flag("--enable_network", appArgs.enableNetwork, "Wait for an authenticated TCP magic packet");
    app.add_option("--network_key", appArgs.networkKey, "Shared key required for network activation");
    app.add_option("--port", appArgs.port, "TCP listening port (default: 47621)");
    app.add_option("--network_timeout", appArgs.networkTimeout, "Maximum request receive time per client (ms; default: 5000)");

    int nArgs;
    LPWSTR* szArglist = CommandLineToArgvW(GetCommandLineW(), &nArgs);
    CLI11_PARSE(app, nArgs, szArglist);

    if (appArgs.enableNetwork && appArgs.networkKey.empty()) {
        std::cerr << "--network_key must be provided and non-empty when --enable_network is used.\n";
        return 1;
    }
    if (appArgs.port < 1 || appArgs.port > 65535) {
        std::cerr << "--port must be in the range 1 to 65535.\n";
        return 1;
    }
    if (appArgs.networkTimeout < 1 || appArgs.networkTimeout > 60000) {
        std::cerr << "--network_timeout must be in the range 1 to 60000 milliseconds.\n";
        return 1;
    }

    DEBUG_INFO("sleep_for: {}", std::to_string(appArgs.time));
    DEBUG_INFO("exit_time: {}", std::to_string(appArgs.exitTime));
    DEBUG_INFO("disable_input: {}", btos(appArgs.disableInput));
    DEBUG_INFO("disable_keyboard: {}", btos(appArgs.disableKeyboard));
    DEBUG_INFO("disable_mouse: {}", btos(appArgs.disableMouse));
    DEBUG_INFO("kill_expoler: {}", btos(appArgs.killExpolerOnStart));
    DEBUG_INFO("topmost: {}", btos(appArgs.topmost));
    DEBUG_INFO("startup: '{}'", appArgs.startupArgs);
    DEBUG_INFO("enable_network: '{}'", appArgs.enableNetwork);
    DEBUG_INFO("network_key: '{}'", appArgs.networkKey);
    DEBUG_INFO("port: '{}'", appArgs.port);
    DEBUG_INFO("network_timeout: '{}'", appArgs.networkTimeout);

    return 0;
}

/// <summary>
/// Terminates all expoler processes
/// </summary>
/// <returns>true on success</returns>
static bool TerminateAllExplorerProcesses() {
    HANDLE hProcessSnap;
    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);

    // All processes snapshot
    hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == INVALID_HANDLE_VALUE) {
        DEBUG_ERR("CreateToolhelp32Snapshot (of processes) failed with error: {}", GetLastError());
        return false;
    }

    // Get 1st proc info
    if (!Process32First(hProcessSnap, &pe32)) {
        DEBUG_ERR("Process32First failed with error: {}", GetLastError());
        CloseHandle(hProcessSnap);
        return false;
    }

    // Walk the snapshot of processes
    do {
        if (_tcsicmp(pe32.szExeFile, _T("explorer.exe")) == 0) {
            HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pe32.th32ProcessID);
            if (hProcess == NULL) {
                DEBUG_ERR("OpenProcess failed with error: {}", GetLastError());
                CloseHandle(hProcessSnap);
                return false;
            }

            // Terminate the process
            if (!TerminateProcess(hProcess, 1)) {
                DEBUG_ERR("TerminateProcess failed with error: {}", GetLastError());
                CloseHandle(hProcess);
                CloseHandle(hProcessSnap);
                return false;
            }

            CloseHandle(hProcess);
        }
    } while (Process32Next(hProcessSnap, &pe32));

    CloseHandle(hProcessSnap);
    return true;
}

/// <summary>
/// Restarts root explorer proc
/// </summary>
/// <returns>true on success</returns>
static bool StartExplorerProcess() {
    wchar_t expPath[] = {L"C:\\Windows\\explorer.exe"};

    STARTUPINFO si = { sizeof(STARTUPINFO) };
    PROCESS_INFORMATION pi;

    if (!CreateProcessW(NULL, expPath, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        DEBUG_ERR("CreateProcess failed with error: {}", GetLastError());
        return false;
    }

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return true;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
    using namespace StartOnBoot;

    if (StartUpLinkExits())
        CleanUpStartOnBoot();

    AppArgs appArgs;
    if (ParseArgs(appArgs) != 0)
    {
        DEBUG_ERR("error parsing args");
        std::cerr << "Faild to parse args" << std::endl;
        return 1;
    }

    if (!appArgs.startupArgs.empty())
    {
        const HRESULT hr = SetupStartOnBoot(appArgs.startupArgs);
        if(hr == S_OK)
            std::cout << "The program will be run on next boot with '" << appArgs.startupArgs << "' args." << std::endl;
        else {
            std::cerr << "Failed to create necessary files to run on next boot!" << std::endl;
        }

        return hr == S_OK;
    }

    if (appArgs.enableNetwork) {
        // Create the thread message queue before the worker can post its activation event.
        MSG queuedMessage{};
        PeekMessage(&queuedMessage, nullptr, WM_USER, WM_USER, PM_NOREMOVE);

        const DWORD uiThreadId = GetCurrentThreadId();
        NetworkTriggerServer server(appArgs.port, appArgs.networkTimeout, std::move(appArgs.networkKey), 
            [uiThreadId] { PostThreadMessageW(uiThreadId, WM_NETWORK_TRIGGER, 0, 0); }
        );

        std::string startupError;
        if (!server.Start(startupError)) {
            MessageBoxA(HWND_DESKTOP, startupError.c_str(), "Network listener error", MB_ICONERROR | MB_OK);
            return 1;
        }

        bool activated = false;
        MSG triggerMessage{};
        while (!activated) {
            const BOOL result = GetMessageW(&triggerMessage, nullptr, 0, 0);

            if (result <= 0)
                return result == 0 ? 0 : 1;

            if (triggerMessage.message == WM_NETWORK_TRIGGER) {
                activated = true;
            }
            else {
                TranslateMessage(&triggerMessage);
                DispatchMessageW(&triggerMessage);
            }
        }
        server.Stop();
    }

    // Wait before the effect starts, whether started automatically or by network request.
    std::this_thread::sleep_for(std::chrono::milliseconds(appArgs.time));

    // Setting up window
    SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_SYSTEM_AWARE);

    screen = {
        GetSystemMetrics(SM_XVIRTUALSCREEN), // a |----------------------|
        GetSystemMetrics(SM_YVIRTUALSCREEN), // b |a:b       |           |
        GetSystemMetrics(SM_CXVIRTUALSCREEN),// c |          |        c:d|
        GetSystemMetrics(SM_CYVIRTUALSCREEN) // d |----------------------|
    };

    WNDCLASS wndClass = { 0, MelterProc, 0, 0, hInstance,
        LoadIcon(NULL, IDI_APPLICATION), LoadCursor(NULL, NULL),
        (HBRUSH)(COLOR_BACKGROUND + 1), 0, L"Melter" };

    if (!RegisterClass(&wndClass))
        return MessageBoxA(HWND_DESKTOP, "Cannot register class!", NULL, MB_ICONERROR | MB_OK);

    DWORD dwExStyle = appArgs.topmost ? WS_EX_TOPMOST : 0;
    HWND hWnd = CreateWindowExA(dwExStyle, "Melter", NULL, WS_POPUP, screen.w, screen.x, screen.y, screen.z, HWND_DESKTOP, NULL, hInstance, NULL);
    if (!hWnd)
        return MessageBoxA(HWND_DESKTOP, "Cannot create window!", NULL, MB_ICONERROR | MB_OK);

#ifndef _DEBUG
    if (appArgs.disableKeyboard || appArgs.disableInput)
        keyboardHhook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardHook, hInstance, 0);

    if (appArgs.disableMouse || appArgs.disableInput)
        ShowCursor(false);
#endif

    bool exitBool = false;
    // This thread will wait for appArgs.exitTime and then set the bool to true
    std::thread exitThread([&appArgs , &exitBool] {
        if (appArgs.exitTime > -1)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(appArgs.exitTime));
            exitBool = true;
        }
    });

    if (appArgs.killExpolerOnStart)
        TerminateAllExplorerProcesses();

    srand((unsigned int)GetTickCount64());
    MSG msg = {0};
    while(msg.message != WM_QUIT)
    {
        if(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        // If the the thread set the bool then destroy win
        if(exitBool)
            DestroyWindow(hWnd);

#ifdef _DEBUG
        if(GetAsyncKeyState(VK_ESCAPE) & 0x8000)
            DestroyWindow(hWnd);
#endif

#ifndef _DEBUG
        if(appArgs.disableMouse || appArgs.disableInput)
            SetCursorPos(0, 0);
#endif
    }

    if (appArgs.killExpolerOnStart)
        StartExplorerProcess();

    exitThread.join();
    return 0;
}
