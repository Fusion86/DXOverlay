#include <dwmapi.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <iostream>
#include <sstream>
#include <Windows.h>

#define TARGET_WINDOW_NAME L"MyWindow"

#define DBOUT(s) \
{ \
	std::wostringstream os_; \
	os_ << s; \
	OutputDebugStringW( os_.str().c_str() ); \
}

HWND g_hWnd;
UINT g_width;
UINT g_height;
RECT g_rc;
HWND g_hTargetWnd;
MARGINS g_margin;
LPDIRECT3D9 g_d3d;
LPDIRECT3DDEVICE9 g_d3ddev;
LPD3DXFONT g_pFont;

void UpdateWHM() {
	GetWindowRect(g_hTargetWnd, &g_rc);
	g_width = g_rc.right - g_rc.left;
	g_height = g_rc.bottom - g_rc.top;
	g_margin = { 0,0, (int)g_width, (int)g_height };
}

void InitDirect3D() {
	g_d3d = Direct3DCreate9(D3D_SDK_VERSION);

	D3DPRESENT_PARAMETERS d3dpp = {};
	d3dpp.Windowed = true;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.hDeviceWindow = g_hWnd;
	d3dpp.BackBufferFormat = D3DFMT_X8R8G8B8;
	d3dpp.BackBufferWidth = g_width;
	d3dpp.BackBufferHeight = g_height;

	d3dpp.EnableAutoDepthStencil = true;
	d3dpp.AutoDepthStencilFormat = D3DFMT_D16;

	g_d3d->CreateDevice(D3DADAPTER_DEFAULT,
		D3DDEVTYPE_HAL,
		g_hWnd,
		D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &g_d3ddev);

	D3DXCreateFont(g_d3ddev, 50, 0, FW_BOLD, 1, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Arial", &g_pFont);
}

void DrawString(int x, int y, DWORD color, LPD3DXFONT pFont, LPCWSTR str)
{
	RECT pos = { x, y, x + 120, y + 16 };
	pFont->DrawText(NULL, str, -1, &pos, DT_NOCLIP, color);
}

void DrawRect(int x, int y, int w, int h, DWORD color) {
	D3DRECT BarRect = { x, y, x + w, y + h };
	g_d3ddev->Clear(1, &BarRect, D3DCLEAR_TARGET | D3DCLEAR_TARGET, color, 0, 0);
}

void Update() {
	// Handle keypresses etc
}

void Render() {
	g_d3ddev->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_ARGB(255, 0, 0, 0), 1.0f, 0);

	g_d3ddev->BeginScene();

	DrawRect(200, 200, 280, 50, D3DCOLOR_ARGB(255, 255, 0, 0));
	DrawString(200, 200, D3DCOLOR_ARGB(255, 255, 255, 255), g_pFont, L"Hello overlay!");

	g_d3ddev->EndScene();

	g_d3ddev->Present(NULL, NULL, NULL, NULL);
}

LRESULT WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
		case WM_PAINT:
			DwmExtendFrameIntoClientArea(hWnd, &g_margin);
			break;

		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;
	}

	return DefWindowProc(hWnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
	// Find target window
	g_hTargetWnd = FindWindow(NULL, TARGET_WINDOW_NAME);
	if (g_hTargetWnd) {
		UpdateWHM();
	}
	else {
		MessageBox(NULL,
			L"Target window not found!",
			L"DXOverlay",
			NULL);
		return EXIT_FAILURE;
	}

	// Create window class
	LPCWSTR windowClassName = L"DXOverlayWindowClass";
	WNDCLASSEX wcex = {};
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WindowProc;
	wcex.hInstance = hInstance;
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)RGB(0, 0, 0);
	wcex.lpszClassName = windowClassName;

	if (!RegisterClassEx(&wcex)) {
		MessageBox(NULL,
			L"Failed to register window class!",
			L"DXOverlay",
			NULL);

		return EXIT_FAILURE;
	}

	// Create overlay window
	g_hWnd = CreateWindow(windowClassName,
		L"DXOverlay Window",
		WS_EX_TOPMOST | WS_POPUP,
		g_rc.left, g_rc.top,
		g_width, g_height,
		NULL,
		NULL,
		hInstance,
		NULL);

	if (!g_hWnd) {
		MessageBox(NULL,
			L"Failed to create window!",
			L"DXHelper",
			NULL);

		return EXIT_FAILURE;
	}

	// Magic
	SetWindowLong(g_hWnd, GWL_EXSTYLE, GetWindowLong(g_hWnd, GWL_EXSTYLE) | WS_EX_LAYERED | WS_EX_TRANSPARENT);
	SetLayeredWindowAttributes(g_hWnd, RGB(0, 0, 0), 0, ULW_COLORKEY);
	SetLayeredWindowAttributes(g_hWnd, 0, 255, LWA_ALPHA);

	ShowWindow(g_hWnd, SW_SHOW);

	InitDirect3D();

	// Make target window not topmost
	SetWindowPos(g_hTargetWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

	MSG msg = { 0 };
	while (WM_QUIT != msg.message) {
		if (PeekMessage(&msg, NULL, NULL, NULL, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		if (IsWindow(g_hTargetWnd)) {
			// hTargetWnd is still open
			if (GetForegroundWindow() == g_hTargetWnd) {
				// hTargetWnd is active

				// TODO: Only call when needed (after resize or after window is 'un-minimized' after being minimized)
				// Currently disabled because it flickers the screen
				//ShowWindow(g_hWnd, SW_SHOW);
				//SetWindowLong(g_hWnd, GWL_EXSTYLE, GetWindowLong(g_hWnd, GWL_EXSTYLE) | WS_EX_LAYERED | WS_EX_TRANSPARENT);
				//SetLayeredWindowAttributes(g_hWnd, RGB(0, 0, 0), 0, ULW_COLORKEY);
				//SetLayeredWindowAttributes(g_hWnd, 0, 255, LWA_ALPHA);

				UpdateWHM();
				SetWindowPos(g_hWnd, HWND_TOPMOST, g_rc.left, g_rc.top, g_width, g_height, NULL);
				Update();
				Render();
			}
			else {
				// hTargetWnd is not active, so no reason to render/update anything
			}
		}
		else {
			// hTargetWnd is closed, so lets close the overlay too
			break;
		}
	}

	DBOUT("Safe exit\n");

	if (g_d3ddev) g_d3ddev->Release();
	if (g_d3d) g_d3d->Release();

	return msg.wParam;
}