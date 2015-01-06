#undef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <ddraw.h>
#ifndef UNDER_CE
#include <stdio.h>
#include <stdlib.h>
#include <io.h>
#endif

#include "client_ddraw.h"
#include "client_gdi.h"
#include "config_storage.h"

#define INITGUID

static const wchar_t *APP_TITLE  = TEXT("VncViewer");
static const wchar_t *APP_NAME = TEXT("avic_vncviewer");

HANDLE g_hMutex;
HWND g_hWndMain;
Client_WinCE *g_Client;
ConfigStorage *g_Config;

Cleaunup()
{
	if (g_hMutex)
	{
		CloseHandle(g_hMutex);
	}
	if (g_Client)
	{
		delete g_Client;
		g_Client = NULL;
	}
	if (g_Config)
	{
		delete g_Config;
		g_Config = NULL;
	}
	DestroyWindow(g_hWndMain);
}

long FAR PASCAL MainWndproc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_ACTIVATE:
		{
			break;
		}
		case WM_SETCURSOR:
		{
			/* Turn off the cursor since this is a full-screen app */
			SetCursor(NULL);
			return TRUE;
		}
		case WM_ERASEBKGND:
		{
			return 1;
		}
		case WM_PAINT:
		{
			return 0;
		}
		case WM_CLOSE:
		{
			DestroyWindow(hWnd);
			return 0;
		}
		case WM_DESTROY:
		{
			PostQuitMessage(0);
			return 0;
		}
		case WM_KEYDOWN:
		{
			switch (wParam)
			{
				case VK_UP:
				{
					break;
				}
				default:
				{
					break;
				}
			}
			break;
		}
		case WM_KEYUP:
		{
			switch (wParam)
			{
				case VK_UP:
				{
					break;
				}
				default:
				{
					break;
				}
			}
			break;
		}
		default:
		{
			break;
		}
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}

bool isAlreadyRunning()
{
	/* allow only single instance */
	g_hMutex = CreateMutex(NULL, FALSE, APP_NAME);
	if (!g_hMutex)
	{
		MessageBox(NULL, TEXT("Failed to create named mutex\r\n") \
			TEXT("Unable to check if single instance is running... Will exit now"),
			TEXT("Error"), MB_OK);
		return TRUE;
	}
	if (ERROR_ALREADY_EXISTS == GetLastError()) {
		HWND hWnd = FindWindow(0, APP_TITLE);
		DEBUGMSG(TRUE, (_T("Found running instance, will exit now\r\n")));
		SetForegroundWindow(hWnd);
		return TRUE;
	}
	return FALSE;
}

bool initialize(HANDLE hInstance, int nCmdShow)
{
	WNDCLASS wc;
	bool rc;

	wc.style = CS_DBLCLKS;
	wc.lpfnWndProc = MainWndproc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(hInstance, IDI_APPLICATION);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = GetStockObject(BLACK_BRUSH);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = TEXT("VncViewerClass");
	rc = RegisterClass(&wc);
	if (!rc)
	{
		return false;
	}

	g_hWndMain = CreateWindowEx(WS_EX_TOPMOST, wc.lpszClassName, APP_TITLE,
		WS_VISIBLE | /* so we don't have to call ShowWindow */
		WS_POPUP, /* non-app window */
		0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN),
		NULL, NULL, hInstance, NULL);
	if (!g_hWndMain)
	{
		return false;
	}
	SetFocus(g_hWndMain);
	return true;

}

bool initializeClient()
{
	wchar_t wfilename[MAX_PATH + 1];
	char filename[MAX_PATH + 1];

	GetModuleFileName(NULL, wfilename, MAX_PATH);
	wcstombs(filename, wfilename, sizeof(filename));
	std::string exe(filename);
	char *dot = strrchr(filename, '.');
	if (dot)
	{
		*dot = '\0';
	}
	std::string ini(filename);
	ini += ".ini";

	g_Config = ConfigStorage::GetInstance();
	g_Config->Initialize(exe, ini);
	switch (g_Config->GetDrawingMethod())
	{
		case 0:
		{
			/* GDI - default */
			g_Client = new Client_GDI();
			break;
		}
		case 1:
		{
			/* DDraw */
			g_Client = new Client_DDraw();
			break;
		}
		default:
		{
			return false;
		}
	}
	g_Client->SetWindowHandle(g_hWndMain);
	return g_Client->Initialize(g_Client) == 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	if (isAlreadyRunning())
	{
		return 0;
	}
	if (!initialize(hInstance, nCmdShow))
	{
		Cleanup();
		return FALSE;
	}
	if (!initializeClient())
	{
		Cleaunup();
		return FALSE;
	}

	bool ret;
	MSG msg;
	while ((ret = GetMessage(&msg, g_hWndMain, 0, 0)) != 0)
	{
		if (ret == -1)
		{
			break;
		}
		else
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	Cleaunup();
	return 0;
}
