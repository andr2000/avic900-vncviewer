#include "client_ddraw.h"

Client_DDraw::Client_DDraw() : Client_WinCE()
{
	lpDD = NULL;
	lpBlitSurface = NULL;
	lpFrontBuffer = NULL;
	lpClipper = NULL;
	m_Initialized = false;
	m_CooperativeLevel = DDSCL_NORMAL;
}

BOOL Client_DDraw::Blit(int x, int y, int w, int h)
{
	HDC hdc;
	while (true)
	{
		HRESULT ddrval = lpBlitSurface->GetDC(&hdc);
		if (ddrval == DD_OK)
		{
			break;
		}
		if (ddrval == DDERR_SURFACELOST)
		{
			if (!RestoreSurfaces())
			{
				return false;
			}
		}
	}
	BOOL result;
	if (m_NeedScaling)
	{
		/* blit all */
		result = StretchBlt(hdc, m_WindowRect.left, m_WindowRect.top,
			m_WindowRect.right - m_WindowRect.left,
			m_WindowRect.bottom - m_WindowRect.top,
			hdcImage, 0, 0, m_Client->width, m_Client->height, SRCCOPY);
	}
	else
	{
		result = BitBlt(hdc, x, y, w, h, hdcImage, x, y, SRCCOPY);
	}
	lpBlitSurface->ReleaseDC(hdc);
	return result;
}

void Client_DDraw::OnFinishedFrameBufferUpdate(rfbClient *client)
{
	if (m_Active)
	{
		int w = m_UpdateRect.x2 - m_UpdateRect.x1;
		int h = m_UpdateRect.y2 - m_UpdateRect.y1;
		DEBUGMSG(TRUE, (TEXT("OnFinishedFrameBufferUpdate: x=%d y=%d w=%d h=%d\r\n"),
			m_UpdateRect.x1, m_UpdateRect.y1, w, h));
		/* blit the update we have just received */
		Blit(m_UpdateRect.x1, m_UpdateRect.y1, w, h);
	}
	Client_WinCE::OnFinishedFrameBufferUpdate(client);
}

void Client_DDraw::OnShutdown()
{
	ReleaseResources();
	Client_WinCE::OnShutdown();
}

int Client_DDraw::SetupClipper()
{
	/* create clipper */
	HRESULT ddrval = lpDD->CreateClipper(0, &lpClipper, NULL);
	if (ddrval != DD_OK)
	{
		DEBUGMSG(TRUE, (TEXT("CreateClipper Failed!\r\n")));
		ReleaseResources();
		return -1;
	}
	ddrval = lpClipper->SetHWnd(0, m_hWnd);
	if (ddrval != DD_OK)
	{
		DEBUGMSG(TRUE, (TEXT("Clipper SetHWnd Failed!\r\n")));
		ReleaseResources();
		return -1;
	}
	ddrval = lpFrontBuffer->SetClipper(lpClipper);
	if (ddrval != DD_OK)
	{
		DEBUGMSG(TRUE, (TEXT("SetClipper Failed!\r\n")));
		ReleaseResources();
		return -1;
	}
	return 0;
}

int Client_DDraw::CreateSurface()
{
	DDSURFACEDESC2 ddsd;
	memset(&ddsd, 0, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);
	ddsd.dwFlags = DDSD_CAPS;
	ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
	HRESULT ddrval = lpDD->CreateSurface(&ddsd, &lpFrontBuffer, NULL);
	if (ddrval != DD_OK)
	{
		DEBUGMSG(TRUE, (TEXT("CreateSurface FrontBuffer Failed!\r\n")));
		ReleaseResources();
		return -1;
	}
	lpBlitSurface = lpFrontBuffer;
	return 0;
}

int Client_DDraw::Initialize()
{
	if (Client_WinCE::Initialize() < 0)
	{
		return -1;
	}
	LPDIRECTDRAW pDD;
	HRESULT ddrval = DirectDrawCreate(NULL, &pDD, NULL);
	if (ddrval != DD_OK)
	{
		DEBUGMSG(TRUE, (TEXT("DirectDrawCreate Failed!\r\n")));
		ReleaseResources();
		return -1;
	}
	/* Fetch DirectDraw4 interface */
	ddrval = pDD->QueryInterface(IID_IDirectDraw4, (LPVOID*) &lpDD);
	if (ddrval != DD_OK)
	{
		DEBUGMSG(TRUE, (TEXT("QueryInterface Failed!\r\n")));
		ReleaseResources();
		return -1;
	}
	pDD->Release();
	pDD = NULL;

	ddrval = lpDD->SetCooperativeLevel(m_hWnd, m_CooperativeLevel);
	if (ddrval != DD_OK)
	{
		DEBUGMSG(TRUE, (TEXT("SetCooperativeLevel Failed!\r\n")));
		ReleaseResources();
		return -1;
	}
	/* Create surface */
	if (CreateSurface() < 0)
	{
		return -1;
	}
	if (SetupClipper() < 0)
	{
		return -1;
	}
	m_Initialized = true;
	return 0;
}

bool Client_DDraw::RestoreSurfaces(void)
{
	HRESULT ddrval = lpFrontBuffer->Restore();
	if (ddrval != DD_OK)
	{
		return false;
	}
	return true;
}

void Client_DDraw::ReleaseResources(void)
{
	if (lpClipper != NULL)
	{
		if (lpFrontBuffer != NULL)
		{
			lpFrontBuffer->SetClipper(NULL);
		}
		lpClipper->Release();
		lpClipper = NULL;
	}
	if (lpFrontBuffer != NULL)
	{
		lpFrontBuffer->Release();
		lpFrontBuffer = NULL;
	}
	if (lpDD != NULL)
	{
		lpDD->SetCooperativeLevel(m_hWnd, DDSCL_NORMAL);
		lpDD->Release();
		lpDD = NULL;
	}
}

void Client_DDraw::OnActivate(bool isActive)
{
	Client_WinCE::OnActivate(isActive);
	DEBUGMSG(TRUE, (TEXT("OnActivate isActive %d m_Initialized %d!\r\n"), isActive, m_Initialized));
	if (isActive && m_Initialized)
	{
		/* framebuffer might already have been updated */
		Blit(m_WindowRect.left, m_WindowRect.top, m_WindowRect.right - m_WindowRect.left,
			m_WindowRect.bottom - m_WindowRect.top);
	}
}

void Client_DDraw::OnPaint(void)
{
	/* we are running a normal window, so we
	 can be inactive, but still partially visible */
	PAINTSTRUCT ps;
	BeginPaint(m_hWnd, &ps);
	LONG x, y, w, h;

	x = ps.rcPaint.left;
	y = ps.rcPaint.top;
	w = ps.rcPaint.right - ps.rcPaint.left;
	h = ps.rcPaint.bottom - ps.rcPaint.top;
	Blit(x, y, w, h);
	EndPaint(m_hWnd, &ps);
}

void Client_DDraw::ShowFullScreen(bool fullScreen)
{
	if ((lpDD == NULL) || (!m_Initialized))
	{
		return;
	}
	if (fullScreen)
	{
		HRESULT ddrval = lpDD->SetCooperativeLevel(m_hWnd, m_CooperativeLevel);
		DEBUGMSG(ddrval != DD_OK, (TEXT("SetCooperativeLevel Failed!\r\n")));
		bool result = RestoreSurfaces();
		DEBUGMSG(!result, (TEXT("RestoreSurfaces Failed!\r\n")));
	}
	else
	{
		lpDD->SetCooperativeLevel(m_hWnd, DDSCL_NORMAL);
	}
}
