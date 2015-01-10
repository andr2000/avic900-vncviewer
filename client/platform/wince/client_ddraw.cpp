#include "client_ddraw.h"

void Client_DDraw::Blit(int x, int y, int w, int h)
{
	HRESULT ddrval;
	HDC hdc;
	if ((ddrval = lpFrontBuffer->GetDC(&hdc)) == DD_OK)
	{
		if (m_NeedScaling)
		{
			/* blit all */
			StretchBlt(hdc, m_WindowRect.left, m_WindowRect.top,
				m_WindowRect.right - m_WindowRect.left,
				m_WindowRect.bottom - m_WindowRect.top,
				hdcImage, 0, 0, m_Client->width, m_Client->height, SRCCOPY);
		}
		else
		{
			BitBlt(hdc, x, y, w, h, hdcImage, x, y, SRCCOPY);
		}
		lpFrontBuffer->ReleaseDC(hdc);
	}
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

int Client_DDraw::Initialize()
{
	lpDD = NULL;
	lpFrontBuffer = NULL;
	lpClipper = NULL;

	if (Client_WinCE::Initialize() < 0)
	{
		return -1;
	}

	HRESULT ddrval;
	LPDIRECTDRAW pDD;
	ddrval = DirectDrawCreate(NULL, &pDD, NULL);
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

	/* windowed mode, so we can switch to other GDI aps */
	ddrval = lpDD->SetCooperativeLevel(m_hWnd, DDSCL_NORMAL);
	if (ddrval != DD_OK)
	{
		DEBUGMSG(TRUE, (TEXT("SetCooperativeLevel Failed!\r\n")));
		ReleaseResources();
		return -1;
	}
	/* Create surface */
	DDSURFACEDESC2 ddsd;
	memset(&ddsd, 0, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);
	ddsd.dwFlags = DDSD_CAPS;
	ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
	ddrval = lpDD->CreateSurface(&ddsd, &lpFrontBuffer, NULL);
	if (ddrval != DD_OK)
	{
		DEBUGMSG(TRUE, (TEXT("CreateSurface FrontBuffer Failed!\r\n")));
		ReleaseResources();
		return -1;
	}
	/* create clipper */
	ddrval = lpDD->CreateClipper(0, &lpClipper, NULL);
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
		lpDD->Release();
		lpDD = NULL;
	}
}

void Client_DDraw::OnActivate(bool isActive)
{
	Client_WinCE::OnActivate(isActive);
	if (isActive)
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
