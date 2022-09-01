// Modified by weyh
// Inspired by Napalm/MetalHead (http://www.rohitab.com/discuss/topic/23191-screen-melter/?p=190669)

#include "framework.h"
#include "main.h"
#include "debug.h"

#include "CLI11.hpp"
#include "start_on_boot.h"

#include <iostream>
#include <chrono>
#include <thread>
#include <string>

int	meltWidth = 150,
    meltHeight = 15,
    interval = 1;
Vector4<int> screen;
HHOOK keyboardHhook;

LRESULT CALLBACK KeyboardHook(int code, WPARAM wParam, LPARAM lParam)
{
    KBDLLHOOKSTRUCT* kbd = (KBDLLHOOKSTRUCT*)lParam;

    // ignore injected events
    if(code < 0 || (kbd->flags & 0x10))
        return CallNextHookEx(keyboardHhook, code, wParam, lParam);

    if(WM_KEYDOWN == wParam)
        debug::WriteLog("WM_KEYDOWN");

    return 1; // by default swallow the keys
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
                int	x = (rand() % screen.y) - (meltWidth / 2),
                    y = (rand() % meltHeight),
                    width = (rand() % meltWidth);
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

    app.add_option("-B,--start_on_boot", appArgs.startupArgs, "Run screen melter on next boot with given args.");

    app.add_flag("-I,--disable_input", appArgs.disableInput, "Disable user input.");
    app.add_flag("-K,--disable_keyboard", appArgs.disableKeyboard, "Disable user keyboard.");
    app.add_flag("-M,--disable_mouse", appArgs.disableMouse, "Disable user mouse.");

    CLI11_PARSE(app, __argc, __argv);

    debug::WriteLog("sleep_for: " + std::to_string(appArgs.time));
    debug::WriteLog("exit_time: " + std::to_string(appArgs.exitTime));
    debug::WriteLog("disable_input: " + btos(appArgs.disableInput));
    debug::WriteLog("disable_keyboard: " + btos(appArgs.disableKeyboard));
    debug::WriteLog("disable_mouse: " + btos(appArgs.disableMouse));
    debug::WriteLog("startup: " + appArgs.startupArgs);

    return 0;
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrev, LPSTR lpCmdLine, int nShowCmd)
{
    using namespace StartOnBoot;

    if (StartUpLinkExits())
        CleanUpStartOnBoot();

    AppArgs appArgs;
    if (ParseArgs(appArgs) != 0)
    {
        debug::WriteLog("error parsing args");
        return 1;
    }

    if (!appArgs.startupArgs.empty())
    {
        HRESULT hr = SetupStartOnBoot(appArgs.startupArgs);
        if(hr == S_OK)
            std::cout << "The will be run on next boot with '" << appArgs.startupArgs << "' args." << std::endl;
        else
            std::cerr << "Failt to create nesesry files to run on next boot!" << std::endl;
        return hr == S_OK;
    }

    // Wait before the effect starts
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

    HWND hWnd = CreateWindowA("Melter", NULL, WS_POPUP, screen.w, screen.x, screen.y, screen.z, HWND_DESKTOP, NULL, hInstance, NULL);
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


    srand(GetTickCount64());
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

    exitThread.join();
    return 0;
}
