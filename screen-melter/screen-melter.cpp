//
// Modified by weyh
// Inspired by Napalm/MetalHead (http://www.rohitab.com/discuss/index.php?showtopic=23191)
//

#include <iostream>
#include <Windows.h>

int	meltWidth = 150,
	meltHeight = 15,
	meltSpeed = 1; // speed
int	screenWidth, screenHeight;

LRESULT WINAPI MelterProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch(Msg)
	{
		case WM_CREATE:
		{
			HDC hdcDesktop = GetDC(HWND_DESKTOP);
			HDC hdcWindow = GetDC(hWnd);
			BitBlt(hdcWindow, 0, 0, screenWidth, screenHeight, hdcDesktop, 0, 0, SRCCOPY);
			ReleaseDC(hWnd, hdcWindow);
			ReleaseDC(HWND_DESKTOP, hdcDesktop);
			SetTimer(hWnd, 0, meltSpeed, NULL);
			ShowWindow(hWnd, SW_SHOW);
		}
		return 0;
		case WM_ERASEBKGND:
			return 0;
		case WM_PAINT:
			//ValidateRect(hWnd, NULL);
		{
			PAINTSTRUCT ps;
			auto hdc = BeginPaint(hWnd, &ps);
			RECT rc;
			GetClientRect(hWnd, &rc);

			HDC hdesktop = CreateDC(L"DISPLAY", NULL, NULL, NULL);

			int screenx = GetSystemMetrics(SM_XVIRTUALSCREEN);
			int screeny = GetSystemMetrics(SM_YVIRTUALSCREEN);
			int screenw = GetSystemMetrics(SM_CXVIRTUALSCREEN);
			int screenh = GetSystemMetrics(SM_CYVIRTUALSCREEN);

			StretchBlt(hdc, 0, 0, rc.right, rc.bottom,
					   hdesktop, screenx, screeny, screenw, screenh, SRCCOPY);

			ReleaseDC(0, hdesktop);

			EndPaint(hWnd, &ps);
			break;
		}
			return 0;
		case WM_TIMER:
		{
			/*HDC hdcWindow = GetDC(hWnd);
			int	x = (rand() % screenWidth) - (meltWidth / 2),
				y = (rand() % meltHeight),
				width = (rand() % meltWidth);
			BitBlt(hdcWindow, x, y, width, screenHeight, hdcWindow, x, 0, SRCCOPY);
			ReleaseDC(hWnd, hdcWindow);*/
		}
		return 0;
		case WM_CLOSE:
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
	SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_SYSTEM_AWARE);
	LPCWSTR szClassName = L"Melter";

	//screenWidth = GetSystemMetrics(SM_CXSCREEN);
	//screenHeight = GetSystemMetrics(SM_CYSCREEN);
	screenWidth = GetSystemMetrics(SM_CXVIRTUALSCREEN);
	screenHeight = GetSystemMetrics(SM_CYVIRTUALSCREEN);

	WNDCLASS wndClass;

	wndClass.style = 0;
	wndClass.lpfnWndProc = MelterProc;
	wndClass.cbClsExtra = 0;
	wndClass.cbWndExtra = 0;
	wndClass.hInstance = hInstance;
	wndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION); // default icon
	wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);   // default arrow mouse cursor
	wndClass.hbrBackground = (HBRUSH)(COLOR_BACKGROUND + 1);
	wndClass.lpszMenuName = NULL;                     // no menu
	wndClass.lpszClassName = szClassName;
	if(!RegisterClass(&wndClass))
		return MessageBoxA(HWND_DESKTOP, "Cannot register class!", NULL, MB_ICONERROR | MB_OK);

	HWND hWnd = CreateWindowA("Melter", NULL, WS_POPUP, 0, 0, screenWidth, screenHeight, HWND_DESKTOP, NULL, hInstance, NULL);
	if(!hWnd)
		return MessageBoxA(HWND_DESKTOP, "Cannot create window!", NULL, MB_ICONERROR | MB_OK);

	srand(GetTickCount());
	MSG Msg = {0};
	while(Msg.message != WM_QUIT)
	{
		if(PeekMessage(&Msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&Msg);
			DispatchMessage(&Msg);
		}
		if(GetAsyncKeyState(VK_ESCAPE) & 0x8000)
			DestroyWindow(hWnd);
	}
	return 0;
}
