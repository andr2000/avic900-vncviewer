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
#include "client_ddraw_exclusive.h"
#include "client_gdi.h"
#include "config_storage.h"

#define INITGUID

static const wchar_t *APP_TITLE = TEXT("VncViewer");
static const wchar_t *APP_NAME = TEXT("avic_vncviewer");

HANDLE g_hMutex;
HWND g_hWndMain;
Client_WinCE *g_Client;

void Cleanup(void)
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
	DestroyWindow(g_hWndMain);
#if !defined(WINCE) && defined(DEBUG)
	FreeConsole();
#endif
}

#ifdef WIN32
void activateNotepad()
{
	HWND hWnd = FindWindow(0, TEXT("Untitled - Notepad"));
	DEBUGMSG(hWnd == INVALID_HANDLE_VALUE, (TEXT("Couldn't find Notepad window\r\n")));
	if (hWnd != INVALID_HANDLE_VALUE)
	{
		SetForegroundWindow(hWnd);
	}
}
#endif

long FAR PASCAL MainWndproc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
#ifdef WINCE
		case WM_ACTIVATE:
#else
		case WM_ACTIVATEAPP:
#endif
		{
			if (g_Client)
			{
				g_Client->OnActivate(wParam != WA_INACTIVE);
				return 0;
			}
			break;
		}
		case WM_ERASEBKGND:
		{
			return 1;
		}
		case WM_PAINT:
		{
			if (g_Client)
			{
				g_Client->OnPaint();
			}
			else
			{
				PAINTSTRUCT ps;
				BeginPaint(hWnd, &ps);
				EndPaint(hWnd, &ps);
			}
			return 0;
		}
		case WM_LBUTTONUP:
		{
			if (g_Client)
			{
				g_Client->OnTouchUp(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
				return 0;
			}
			break;
		}
		case WM_LBUTTONDOWN:
		{
			if (g_Client)
			{
				g_Client->OnTouchDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
				return 0;
			}
			break;
		}
		case WM_MOUSEMOVE:
		{
			if (g_Client)
			{
				if (wParam & MK_LBUTTON)
				{
					g_Client->OnTouchMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
					return 0;
				}
			}
			break;
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
#ifdef WIN32
		case WM_KEYUP:
		{
			if (wParam == VK_SPACE)
			{
				activateNotepad();
				return 0;
			}
			break;
		}
#endif
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
		MessageBox(NULL, TEXT("Failed to create named mutex\r\n")
			TEXT("Unable to check if single instance is running... Will exit now"),
			TEXT("Error"), MB_OK);
		return TRUE;
	}
	if (ERROR_ALREADY_EXISTS == GetLastError())
	{
		HWND hWnd = FindWindow(0, APP_TITLE);
		DEBUGMSG(TRUE, (TEXT("Found running instance, will exit now\r\n")));
		SetForegroundWindow(hWnd);
		return TRUE;
	}
	return FALSE;
}

bool initialize(HINSTANCE hInstance, int nCmdShow)
{
	WNDCLASS wc;
	bool rc;

	memset(&wc, 0, sizeof(wc));
	wc.style = CS_DBLCLKS;
	wc.lpfnWndProc = MainWndproc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH) GetStockObject(BLACK_BRUSH);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = TEXT("VncViewerClass");
	rc = RegisterClass(&wc) ? true : false;
	if (!rc)
	{
		return false;
	}
	int w, h;
#ifdef WINCE
	w = GetSystemMetrics(SM_CXSCREEN);
	h = GetSystemMetrics(SM_CYSCREEN);
#else
	w = 800;
	h = 480;
#endif
	DWORD extStyle = 0;
	if (ConfigStorage::GetInstance()->GetDrawingMethod() == ConfigStorage::DDRAW_EXCLUSIVE)
	{
		extStyle = WS_EX_TOPMOST;
	}
	g_hWndMain = CreateWindowEx(extStyle, wc.lpszClassName, APP_TITLE, WS_POPUP, /* non-app window */
		0, 0, w, h, NULL, NULL, hInstance, NULL);
	if (!g_hWndMain)
	{
		return false;
	}
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

	ConfigStorage::GetInstance()->Initialize(exe, ini);
	/* do not use global client pointer until we are fully initialized, so
	 window proc doesn't call us yet */
	switch (ConfigStorage::GetInstance()->GetDrawingMethod())
	{
		case ConfigStorage::GDI:
		{
			/* GDI - default */
			g_Client = new Client_GDI();
			break;
		}
		case ConfigStorage::DDRAW_WINDOWED:
		{
			/* DDraw */
			g_Client = new Client_DDraw();
			break;
		}
		case ConfigStorage::DDRAW_EXCLUSIVE:
		{
			/* DDraw exclusive */
			g_Client = new Client_DDraw_Exclusive();
			break;
		}
		default:
		{
			return false;
		}
	}
	g_Client->SetWindowHandle(g_hWndMain);
	if (g_Client->Initialize() < 0)
	{
		return false;
	}
	SetFocus(g_hWndMain);
	ShowWindow(g_hWndMain, SW_SHOW);
	return true;
}

#ifdef WINCE
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
#else
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
#endif
{
	if (isAlreadyRunning())
	{
		return 0;
	}
#if !defined(WINCE) && defined(DEBUG)
	AllocConsole();
	freopen("CONOUT$","w",stdout);
#endif
	if (!initialize(hInstance, nCmdShow))
	{
		Cleanup();
		return FALSE;
	}
	if (!initializeClient())
	{
		Cleanup();
		return FALSE;
	}

	int ret;
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
	Cleanup();
	return 0;
}
