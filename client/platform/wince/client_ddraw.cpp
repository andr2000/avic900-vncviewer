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

void Client_DDraw::BlitFrontToBack(LPRECT rect) {
	while (true)
	{
		HRESULT ddrval = lpBackBuffer->BltFast(rect->left, rect->top,
			lpFrontBuffer, rect, DDBLTFAST_NOCOLORKEY);
		if (ddrval == DD_OK)
		{
			break;
		}
		if (ddrval == DDERR_SURFACELOST)
		{
			if (!RestoreSurfaces())
			{
				return;
			}
			if (ddrval != DDERR_WASSTILLDRAWING)
			{
				return;
			}
		}
	}
}

void Client_DDraw::OnFinishedFrameBufferUpdate(rfbClient *client) {
	int w = m_UpdateRect.x2 - m_UpdateRect.x1;
	int h = m_UpdateRect.y2 - m_UpdateRect.y1;
	DEBUGMSG(TRUE, (_T("OnFinishedFrameBufferUpdate: x=%d y=%d w=%d h=%d\r\n"),
		m_UpdateRect.x1, m_UpdateRect.y1, w, h));

	/* blit from front to back those pixels which will not be updated by the m_UpdateRect */
	RECT rect;
	if (m_UpdateRect.y1 > 0)
	{
		/* have rectangle on top */
		rect.left = rect.top = 0;
		rect.right = m_ClientRect.right;
		rect.bottom = m_UpdateRect.y1;
		BlitFrontToBack(&rect);
	}
	if (m_UpdateRect.y2 < m_ClientRect.bottom)
	{
		/* have rectangle at bottom */
		rect.left = 0;
		rect.top = m_UpdateRect.y2;
		rect.right = m_ClientRect.right;
		rect.bottom = m_ClientRect.bottom;
		BlitFrontToBack(&rect);
	}
	if (m_UpdateRect.x1 > 0)
	{
		/* have rectangle on left */
		rect.left = 0;
		rect.top = m_UpdateRect.y1;
		rect.right = m_UpdateRect.x1;
		rect.bottom = m_UpdateRect.y2;
		BlitFrontToBack(&rect);
	}
	if (m_UpdateRect.x2 < m_ClientRect.right)
	{
		/* have rectangle on right */
		rect.left = m_UpdateRect.x2;
		rect.top = m_UpdateRect.y1;
		rect.right = m_ClientRect.right;
		rect.bottom = m_UpdateRect.y2;
		BlitFrontToBack(&rect);
	}
	/* now blit the update we have just received */
	HRESULT ddrval;
	HDC hdc;
	if ((ddrval = lpBackBuffer->GetDC(&hdc)) == DD_OK)
	{
		if (!BitBlt(hdc, m_UpdateRect.x1, m_UpdateRect.y1, w, h,
			hdcImage, m_UpdateRect.x1, m_UpdateRect.y1, SRCCOPY))
		{
			ddrval = E_FAIL;
		}
		lpBackBuffer->ReleaseDC(hdc);
	}
	/* now flip */
	while (true)
	{
		ddrval = lpFrontBuffer->Flip(NULL, 0);
		if (ddrval == DD_OK)
		{
			break;
		}
		if (ddrval == DDERR_SURFACELOST)
		{
			if (!RestoreSurfaces())
			{
				return;
			}
		}
		if (ddrval != DDERR_WASSTILLDRAWING)
		{
			break;
		}
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
	lpBackBuffer = NULL;

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

	ddrval = lpDD->SetCooperativeLevel(m_hWnd, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
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
	ddsd.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
	ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE |
		DDSCAPS_FLIP | DDSCAPS_COMPLEX;
	ddsd.dwBackBufferCount = 1;
	ddrval = lpDD->CreateSurface(&ddsd, &lpFrontBuffer, NULL);
	if (ddrval != DD_OK)
	{
		if (ddrval == DDERR_NOFLIPHW)
		{
			DEBUGMSG(TRUE, (TEXT("Display driver doesn't support flipping surfaces!\r\n")));
			ReleaseResources();
			return -1;
		}
		DEBUGMSG(TRUE, (TEXT("CreateSurface FrontBuffer Failed!\r\n")));
		ReleaseResources();
		return -1;
	}
	/* get a pointer to the back buffer */
	DDSCAPS2 ddscaps;
	ddscaps.dwCaps = DDSCAPS_BACKBUFFER;
	ddrval = lpFrontBuffer->GetAttachedSurface(&ddscaps, &lpBackBuffer);
	if (ddrval != DD_OK)
	{
		DEBUGMSG(TRUE, (TEXT("GetAttachedSurface Failed!\r\n")));
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
	if (lpBackBuffer != NULL)
	{
		lpBackBuffer->Release();
		lpBackBuffer = NULL;
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
