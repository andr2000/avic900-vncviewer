#include "client_ddraw.h"

rfbBool Client_DDraw::OnMallocFrameBuffer(rfbClient *client)
{
	if (!Client_WinCE::OnMallocFrameBuffer(client))
	{
		return FALSE;
	}
	hdcImage = CreateCompatibleDC(NULL);
	if (!hdcImage)
	{
		return -1;
	}
	SelectObject(hdcImage, m_hBmp);
	return TRUE;
}

void Client_DDraw::Blit(int x, int y, int w, int h)
{
	HRESULT ddrval;
	HDC hdc;
	if ((ddrval = lpFrontBuffer->GetDC(&hdc)) == DD_OK)
	{
		if (!BitBlt(hdc, x, y, w, h, hdcImage, x, y, SRCCOPY))
		{
			ddrval = E_FAIL;
		}
		lpFrontBuffer->ReleaseDC(hdc);
	}
}

void Client_DDraw::OnFinishedFrameBufferUpdate(rfbClient *client) {
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
	DeleteDC(hdcImage);
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
	ddrval = pDD->QueryInterface(IID_IDirectDraw4, (LPVOID*)&lpDD);
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
	ddsd.dwSize = sizeof( ddsd );
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
		Blit(m_ClientRect.left, m_ClientRect.top,
			m_ClientRect.right - m_ClientRect.left,
			m_ClientRect.bottom - m_ClientRect.top);
	}
}
