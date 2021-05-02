// Modified by weyh
// Inspired by Napalm/MetalHead (http://www.rohitab.com/discuss/topic/23191-screen-melter/?p=190669)

#include "framework.h"
#include "screen-melter.h"
#include "Debug.h"

#include "CLI11.hpp"

#include <iostream>
#include <chrono>
#include <thread>
#include <string>

int	meltWidth = 150,
    meltHeight = 15,
    interval = 1;
Vector4Int screen;
HHOOK keyboardHhook;

LRESULT CALLBACK KeyboardHook(int code, WPARAM wParam, LPARAM lParam)
{
    KBDLLHOOKSTRUCT* kbd = (KBDLLHOOKSTRUCT*)lParam;

    // ignore injected events
    if(code < 0 || (kbd->flags & 0x10))
        return CallNextHookEx(keyboardHhook, code, wParam, lParam);

    if(WM_KEYDOWN == wParam)
        Debug::WriteLog("WM_KEYDOWN");

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
        {
            KillTimer(hWnd, 0);
            PostQuitMessage(0);
        }
        return 0;
    }

    return DefWindowProc(hWnd, Msg, wParam, lParam);
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrev, LPSTR lpCmdLine, int nShowCmd)
{
    CLI::App appArgs {"Creates melting like effect on users screen."};

    std::string time = "0";
    appArgs.add_option("-t,--time", time, "Sleep time before visual effect (ms)");

    bool exitBool = false;
    std::string exitTime = "-1"; // -1 -> infinite/not set
    appArgs.add_option("-e,--exit_time", exitTime, "Sleep time before visual effect (ms)");

    bool disableInput = false;
    bool disableKeyboard = false;
    bool disableMouse = false;

    appArgs.add_flag("-I,--disable_input", disableInput, "Disable user input.");
    appArgs.add_flag("-K,--disable_keyboard", disableKeyboard, "Disable user keyboard.");
    appArgs.add_flag("-M,--disable_mouse", disableMouse, "Disable user mouse.");

    CLI11_PARSE(appArgs, __argc, __argv);

    Debug::WriteLog("sleep_for: " + time);
    Debug::WriteLog("exit_time: " + exitTime);
    Debug::WriteLog("disable_input: " + btos(disableInput));
    Debug::WriteLog("disable_keyboard: " + btos(disableKeyboard));
    Debug::WriteLog("disable_mouse: " + btos(disableMouse));

    std::this_thread::sleep_for(std::chrono::milliseconds(std::stoi(time)));

    SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_SYSTEM_AWARE);

    screen = {
        GetSystemMetrics(SM_XVIRTUALSCREEN), // a |----------------------|
        GetSystemMetrics(SM_YVIRTUALSCREEN), // b |a:b       |           |
        GetSystemMetrics(SM_CXVIRTUALSCREEN),// c |          |        c:d|
        GetSystemMetrics(SM_CYVIRTUALSCREEN) // d |----------------------|
    };

    WNDCLASS wndClass = {0, MelterProc, 0, 0, hInstance,
        LoadIcon(NULL, IDI_APPLICATION), LoadCursor(NULL, NULL),
        (HBRUSH)(COLOR_BACKGROUND + 1), 0, L"Melter"};

    if(!RegisterClass(&wndClass))
        return MessageBoxA(HWND_DESKTOP, "Cannot register class!", NULL, MB_ICONERROR | MB_OK);

    HWND hWnd = CreateWindowA("Melter", NULL, WS_POPUP, screen.w, screen.x, screen.y, screen.z, HWND_DESKTOP, NULL, hInstance, NULL);
    if(!hWnd)
        return MessageBoxA(HWND_DESKTOP, "Cannot create window!", NULL, MB_ICONERROR | MB_OK);

#ifndef _DEBUG
    if(disableKeyboard || disableInput)
        keyboardHhook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardHook, hInstance, 0);

    if(disableMouse || disableInput)
        ShowCursor(false);
#endif

    std::thread exitThread(WaitAndSet, std::stoi(exitTime), &exitBool);

    srand(GetTickCount64());
    MSG Msg = {0};
    while(Msg.message != WM_QUIT)
    {
        if(PeekMessage(&Msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&Msg);
            DispatchMessage(&Msg);
        }

        if(exitBool)
            DestroyWindow(hWnd);

    #ifdef _DEBUG
        if(GetAsyncKeyState(VK_ESCAPE) & 0x8000)
            DestroyWindow(hWnd);
    #endif

    #ifndef _DEBUG
        if(disableMouse || disableInput)
            SetCursorPos(0, 0);
    #endif
    }

    exitThread.join();
    return 0;
}
